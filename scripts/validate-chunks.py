#!/usr/bin/env python3
"""
Download previously-submitted chunks from the matroid enumeration server
and run validation checks on them.

Usage:
    python3 scripts/validate-chunks.py --start N --end M [--base-url URL]
                                       [--delete-on-failure --api-token-file PATH]

Both --start and --end are inclusive.
"""

import argparse
import hashlib
import http.client
import subprocess
import sys
import tempfile
import time
import urllib.error
import urllib.request
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from sz_validation import validate_sz_files

USER_AGENT = "matroid-generator/validate-chunks"
DEFAULT_BASE_URL = "https://matroid-enumeration.icarm.workers.dev"

HTTP_MAX_ATTEMPTS = 4
HTTP_RETRY_DELAY = 2.0  # seconds, doubled on each retry


def urlopen_with_retry(req, chunk_id, what):
    """Open `req`, retrying on 5xx and network errors. Raises on final failure.

    Args:
        req (urllib.request.Request): the HTTP request to open.
        chunk_id (int): chunk index, used only for log messages.
        what (str): short label for the operation, used only for log messages.

    Returns:
        http.client.HTTPResponse
    """
    delay = HTTP_RETRY_DELAY
    for attempt in range(1, HTTP_MAX_ATTEMPTS + 1):
        try:
            return urllib.request.urlopen(req)
        except urllib.error.HTTPError as e:
            if e.code < 500 or attempt == HTTP_MAX_ATTEMPTS:
                raise
            reason = f"returned {e.code}"
        except (urllib.error.URLError, http.client.HTTPException, OSError) as e:
            if attempt == HTTP_MAX_ATTEMPTS:
                raise
            reason = f"network error ({type(e).__name__}: {e})"
        print(
            f"RETRY chunk {chunk_id}: {what} {reason}, "
            f"retrying in {delay:.1f}s ({attempt}/{HTTP_MAX_ATTEMPTS - 1})",
            flush=True,
        )
        time.sleep(delay)
        delay *= 2


def delete_chunk(base_url, chunk_id, xz_hash, api_token):
    """Ask the server to delete a chunk. Returns a status message (str).

    Args:
        base_url (str): server base URL, no trailing slash.
        chunk_id (int): chunk index to delete.
        xz_hash (str): hex sha256 of the .xz file (the server verifies this).
        api_token (str): bearer token for Authorization header.
    """
    url = f"{base_url}/delete-chunk/{chunk_id}?hash={xz_hash}"
    req = urllib.request.Request(
        url,
        method="POST",
        headers={
            "User-Agent": USER_AGENT,
            "Authorization": f"Bearer {api_token}",
        },
    )
    try:
        with urlopen_with_retry(req, chunk_id, "delete") as resp:
            return f"deleted ({resp.read().decode().strip()})"
    except urllib.error.HTTPError as e:
        return f"delete returned {e.code} ({e.read().decode().strip()})"
    except (urllib.error.URLError, http.client.HTTPException, OSError) as e:
        return f"delete error: {type(e).__name__}: {e}"


def validate_one(base_url, chunk_id, delete_on_failure, api_token):
    """Validate one chunk. Prints status lines; returns True on success (bool).

    Args:
        base_url (str): server base URL, no trailing slash.
        chunk_id (int): chunk index to fetch and validate.
        delete_on_failure (bool): if True, request server-side deletion on failure.
        api_token (str or None): bearer token; required iff delete_on_failure.
    """
    url = f"{base_url}/chunk/{chunk_id}"

    def fail(msg, xz_path=None):
        """Log a failure and (optionally) ask the server to delete the chunk.

        Args:
            msg (str): failure reason to include in the log line.
            xz_path (pathlib.Path or None): path to the downloaded .xz file,
                needed to compute the hash for the delete request.
        """
        print(f"FAIL chunk {chunk_id}: {msg}", flush=True)
        if delete_on_failure and xz_path is not None:
            xz_sha = hashlib.sha256()
            with open(xz_path, "rb") as f:
                for block in iter(lambda: f.read(1 << 16), b""):
                    xz_sha.update(block)
            xz_hex = xz_sha.hexdigest()
            status = delete_chunk(base_url, chunk_id, xz_hex, api_token)
            print(f"DELETE chunk {chunk_id} (hash={xz_hex}): {status}", flush=True)
        return False

    with tempfile.TemporaryDirectory() as tmpdir:
        xz_path = Path(tmpdir) / f"n10r05-seedmatroid{chunk_id:06d}.sz.xz"
        sz_path = Path(tmpdir) / f"n10r05-seedmatroid{chunk_id:06d}.sz"

        req = urllib.request.Request(url, headers={"User-Agent": USER_AGENT})
        try:
            with urlopen_with_retry(req, chunk_id, "download") as resp, \
                 open(xz_path, "wb") as f:
                while True:
                    block = resp.read(1 << 16)
                    if not block:
                        break
                    f.write(block)
        except urllib.error.HTTPError as e:
            return fail(f"download returned {e.code}")
        except (urllib.error.URLError, http.client.HTTPException, OSError) as e:
            return fail(f"download error: {type(e).__name__}: {e}")

        try:
            with open(sz_path, "wb") as out:
                subprocess.run(
                    ["xzcat", str(xz_path)],
                    check=True, stdout=out,
                )
        except subprocess.CalledProcessError as e:
            return fail(f"xzcat failed: {e}", xz_path)

        ok, msg, count = validate_sz_files(sz_path, xz_path)

        if ok:
            print(f"OK   chunk {chunk_id}: {count} matroids", flush=True)
            return True

        return fail(msg, xz_path)


def main():
    parser = argparse.ArgumentParser(description="Validate previously-submitted chunks.")
    parser.add_argument("--start", type=int, required=True, help="Start index (inclusive)")
    parser.add_argument("--end", type=int, required=True, help="End index (inclusive)")
    parser.add_argument(
        "--base-url", default=DEFAULT_BASE_URL,
        help=f"Server base URL (default: {DEFAULT_BASE_URL})",
    )
    parser.add_argument(
        "--delete-on-failure", action="store_true",
        help="Ask the server to delete chunks that fail validation",
    )
    parser.add_argument(
        "--api-token-file",
        help="Path to file containing the API bearer token (required with --delete-on-failure)",
    )
    args = parser.parse_args()

    api_token = None
    if args.delete_on_failure:
        if not args.api_token_file:
            parser.error("--delete-on-failure requires --api-token-file")
        api_token = Path(args.api_token_file).read_text().strip()

    total = 0
    failed = 0
    for idx in range(args.start, args.end + 1):
        ok = validate_one(args.base_url, idx, args.delete_on_failure, api_token)
        total += 1
        if not ok:
            failed += 1

    print(f"\nDone: {total} chunks checked, {failed} failed.")
    sys.exit(1 if failed else 0)


if __name__ == "__main__":
    main()
