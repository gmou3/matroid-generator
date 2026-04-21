#!/usr/bin/env python3
"""
Fetch chunk assignments from the matroid enumeration server, run IC to
generate them, compress, and submit the results.

Usage:
    python3 scripts/submit-chunks.py [--workers N] [--chunks N] [--base-url URL]
"""

import argparse
import getpass
import hashlib
import json
import os
import platform
import subprocess
import sys
import urllib.request
import urllib.error
from concurrent.futures import ProcessPoolExecutor, wait, FIRST_COMPLETED
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from sz_validation import validate_sz_files

USER_AGENT = "matroid-generator/submit-chunks"

DEFAULT_BASE_URL = "https://matroid-enumeration.icarm.workers.dev"
PROJECT_ROOT = Path(__file__).resolve().parent.parent
OUTPUT_DIR = PROJECT_ROOT / "output"
IC_BIN = PROJECT_ROOT / "build" / "IC"

MAX_IC_ATTEMPTS = 3


def process_chunk(base_url, api_token, cleanup=False):
    """Get an assignment, run IC, compress, and submit. Returns a status string.

    Args:
        base_url (str): server base URL, no trailing slash.
        api_token (str): bearer token for Authorization header.
        cleanup (bool): if True, delete local .sz / .xz files after successful submit.

    Returns:
        str: a single-line status message prefixed with OK / FAIL / SKIP.
    """

    # 1. Get a chunk assignment.
    req = urllib.request.Request(
        f"{base_url}/new-assignment",
        method="POST",
        headers={
            "User-Agent": USER_AGENT,
            "Authorization": f"Bearer {api_token}",
        },
    )
    try:
        with urllib.request.urlopen(req) as resp:
            body = json.loads(resp.read().decode())
            chunk_id = body["index"]
    except urllib.error.HTTPError as e:
        return f"SKIP: server returned {e.code} for /new-assignment ({e.read().decode().strip()})"

    padded = f"{chunk_id:06d}"
    sz_file = OUTPUT_DIR / f"n10r05-seedmatroid{padded}.sz"
    xz_file = OUTPUT_DIR / f"n10r05-seedmatroid{padded}.sz.xz"

    try:
        # 2. Run IC + compress + validate, with retries if validation fails.
        matroid_count = None
        for attempt in range(1, MAX_IC_ATTEMPTS + 1):
            if not sz_file.exists():
                subprocess.run(
                    [str(IC_BIN), str(chunk_id)],
                    check=True,
                    cwd=str(PROJECT_ROOT),
                )
            if not sz_file.exists():
                return f"FAIL chunk {chunk_id}: IC did not produce {sz_file.name}"

            if not xz_file.exists():
                subprocess.run(
                    ["xz", "-9e", "-k", str(sz_file)],
                    check=True,
                )

            ok, msg, count = validate_sz_files(sz_file, xz_file)
            if ok:
                matroid_count = str(count)
                break

            print(
                f"ERROR chunk {chunk_id} (attempt {attempt}/{MAX_IC_ATTEMPTS}): {msg}; rerunning IC",
                flush=True,
            )
            sz_file.unlink(missing_ok=True)
            xz_file.unlink(missing_ok=True)
        else:
            return f"FAIL chunk {chunk_id}: validation failed after {MAX_IC_ATTEMPTS} attempts"

        # 3. Compute sha256 of the .sz file.
        sz_sha = hashlib.sha256()
        with open(sz_file, "rb") as f:
            for block in iter(lambda: f.read(1 << 16), b""):
                sz_sha.update(block)
        sz_digest_hex = sz_sha.hexdigest()

        # 4. Compute sha256 of the .xz file.
        xz_sha = hashlib.sha256()
        with open(xz_file, "rb") as f:
            for block in iter(lambda: f.read(1 << 16), b""):
                xz_sha.update(block)
        digest_hex = xz_sha.hexdigest()

        # 5. Submit.
        with open(xz_file, "rb") as f:
            body = f.read()

        submit_req = urllib.request.Request(
            f"{base_url}/matroids/{chunk_id}",
            data=body,
            method="POST",
            headers={
                "Content-Type": "application/octet-stream",
                "Content-Digest": f"sha-256={digest_hex}",
                "X-Sz-Hash": sz_digest_hex,
                "X-Matroid-Count": matroid_count,
                "X-User": getpass.getuser(),
                "X-Host": platform.node(),
                "User-Agent": USER_AGENT,
                "Authorization": f"Bearer {api_token}",
            },
        )
        with urllib.request.urlopen(submit_req) as resp:
            resp_body = resp.read().decode().strip()

        if cleanup:
            sz_file.unlink(missing_ok=True)
            xz_file.unlink(missing_ok=True)

        return f"OK   chunk {chunk_id}: submitted ({resp_body})"
    except subprocess.CalledProcessError as e:
        return f"FAIL chunk {chunk_id}: subprocess error: {e}"
    except urllib.error.HTTPError as e:
        return f"FAIL chunk {chunk_id}: submit returned {e.code} ({e.read().decode().strip()})"
    except Exception as e:
        return f"FAIL chunk {chunk_id}: {e}"


def main():
    parser = argparse.ArgumentParser(description="Generate and submit matroid chunks.")
    parser.add_argument(
        "--workers", type=int, default=1,
        help="Number of concurrent subprocesses (default: 1)",
    )
    parser.add_argument(
        "--chunks", type=int, default=1,
        help="Total number of chunks to process before halting (default: 1)",
    )
    parser.add_argument(
        "--base-url", default=DEFAULT_BASE_URL,
        help=f"Server base URL (default: {DEFAULT_BASE_URL})",
    )
    parser.add_argument(
        "--api-token-file", required=True,
        help="Path to file containing the API bearer token",
    )
    parser.add_argument(
        "--cleanup", action="store_true",
        help="Delete local .sz and .xz files after successful submission",
    )
    args = parser.parse_args()

    api_token = Path(args.api_token_file).read_text().strip()

    if not IC_BIN.exists():
        print(f"Error: {IC_BIN} not found. Run 'make' first.", file=sys.stderr)
        sys.exit(1)

    OUTPUT_DIR.mkdir(parents=True, exist_ok=True)

    completed = 0
    failed = 0

    with ProcessPoolExecutor(max_workers=args.workers) as pool:
        pending = set()
        submitted = 0

        # Seed the pool.
        for _ in range(min(args.workers, args.chunks)):
            pending.add(pool.submit(process_chunk, args.base_url, api_token, args.cleanup))
            submitted += 1

        while pending:
            done, pending = wait(pending, return_when=FIRST_COMPLETED)
            for future in done:
                result = future.result()
                print(result, flush=True)

                if result.startswith("FAIL"):
                    failed += 1
                completed += 1

                # Submit more work if we haven't reached the total yet.
                if submitted < args.chunks:
                    pending.add(pool.submit(process_chunk, args.base_url, api_token, args.cleanup))
                    submitted += 1

    print(f"\nDone: {completed} chunks processed, {failed} failed.")


if __name__ == "__main__":
    main()
