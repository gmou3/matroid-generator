"""Shared validation helpers for .sz / .sz.xz files."""

import subprocess
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
SZCAT = PROJECT_ROOT / "scripts" / "szcat.sh"
SZXZCAT = PROJECT_ROOT / "scripts" / "szxzcat.sh"

EXPECTED_LINE_LEN = 252  # C(10, 5)


def parse_sz_info(info):
    """Parse output of 'szcat -i', e.g. '3339058 strings of length 252'.

    Args:
        info (str): the stdout line from 'szcat -i'.

    Returns:
        tuple of (count: int, line_length: int).
    """
    fields = info.split()
    return int(fields[0]), int(fields[4])


def validate_sz_files(sz_file, xz_file):
    """Run sanity checks on .sz and .sz.xz files.

    Args:
        sz_file (pathlib.Path): path to the uncompressed .sz file.
        xz_file (pathlib.Path): path to the .sz.xz file.

    Returns:
        tuple of (ok: bool, message: str, matroid_count: int). On failure,
        message describes the first problem found. A subprocess failure
        (e.g. a corrupted file causing szcat to exit non-zero) is reported
        as a validation failure rather than propagated as an exception.
    """
    try:
        szcat_i = subprocess.run(
            [str(SZCAT), "-i", str(sz_file)],
            check=True, capture_output=True, text=True,
        ).stdout.strip()
        count_i, line_len = parse_sz_info(szcat_i)

        if line_len != EXPECTED_LINE_LEN:
            return False, f"line length is {line_len}, expected {EXPECTED_LINE_LEN}", count_i

        wc_result = subprocess.run(
            f"set -o pipefail; {SZCAT} {sz_file} | wc -l",
            shell=True, check=True, capture_output=True, text=True,
            executable="/bin/bash",
        )
        count_wc = int(wc_result.stdout.strip())
        if count_i != count_wc:
            return False, f"szcat -i count ({count_i}) != szcat | wc -l ({count_wc})", count_i

        szxzcat_i = subprocess.run(
            [str(SZXZCAT), "-i", str(xz_file)],
            check=True, capture_output=True, text=True,
        ).stdout.strip()
        count_xz, _ = parse_sz_info(szxzcat_i)
        if count_i != count_xz:
            return False, f"szcat -i count ({count_i}) != szxzcat -i count ({count_xz})", count_i
    except subprocess.CalledProcessError as e:
        cmd = e.cmd if isinstance(e.cmd, str) else " ".join(e.cmd)
        stderr = (e.stderr or "").strip()
        detail = f": {stderr}" if stderr else ""
        return False, f"'{cmd}' exited with {e.returncode}{detail}", 0
    except (ValueError, IndexError) as e:
        return False, f"could not parse sz info output: {e}", 0

    return True, "", count_i
