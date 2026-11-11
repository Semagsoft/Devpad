#!/usr/bin/env python3
"""
Devpad - A C++/Qt6 code editor
Copyright (C) 2026 Semagsoft

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 2
of the License, or (at your option) any later version.
"""
"""Run clang-tidy over the entries of a CMake compilation database.

Mirrors the CI "Clang-tidy" job: only files with an entry in the
database are linted (sources behind OFF-by-default options such as
BUILD_TUI/BUILD_NEXTGEN have no DB entry and would misparse).

Usage:
    python3 scripts/run-clang-tidy.py --build-dir build

    # What to lint (default: all)
    --src-only / --tests-only

NOTE: clang-tidy must be a version consistent with the project's CI
(CI pins clang-tidy-18). Newer local clang-tidy versions may emit
unrelated warnings or reject toolchain flags from the compile database.
"""

import argparse
import json
import os
import subprocess
import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path


def db_files(build_dir: str, wanted: str):
    db_path = Path(build_dir) / "compile_commands.json"
    if not db_path.exists():
        sys.exit(f"error: {db_path} not found. Configure with -DCMAKE_EXPORT_COMPILE_COMMANDS=ON first.")
    with db_path.open() as f:
        db = json.load(f)
    seen = set()
    for e in db:
        p = e["file"]
        if wanted == "src" and "/src/" not in p:
            continue
        if wanted == "tests" and "/tests/" not in p:
            continue
        low = p.lower()
        if "kodoterm" in low or "qtermwidget" in low or "ghostty" in low:
            continue
        if p.endswith(".cpp"):
            seen.add(p)
    return sorted(seen)


def run_tidy(entry):
    file, build_dir, header_filter = entry
    cmd = [
        "clang-tidy",
        "--quiet",
        "--header-filter=" + header_filter,
        "--extra-arg=-Wno-unknown-argument",
        "-p",
        os.path.abspath(build_dir),
        file,
    ]
    result = subprocess.run(cmd, capture_output=True, text=True)
    if result.returncode != 0:
        print(f"FAILED ({result.returncode}): {file}")
        if result.stdout:
            print(result.stdout)
        if result.stderr:
            print(result.stderr)
        return False
    output = result.stdout.strip()
    if output:
        print(f"--- {file}\n{output}\n")
    return True


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", default=os.environ.get("DEVPAD_BUILD_DIR", "build"))
    parser.add_argument("--src-only", action="store_const", const="src", dest="wanted")
    parser.add_argument("--tests-only", action="store_const", const="tests", dest="wanted")
    parser.set_defaults(wanted="all")
    args = parser.parse_args()

    files = db_files(args.build_dir, args.wanted)
    if not files:
        sys.exit("error: no lintable .cpp entries found in the compilation database")

    source_dir = Path(__file__).resolve().parent.parent
    root = str(source_dir)
    if args.wanted == "src":
        header_filter = f"^{root}/src/.*"
    else:
        # CI lints tests with src+tests headers as well.
        header_filter = f"^{root}/(src|tests)/.*"

    jobs = max(os.cpu_count() or 1, 1)
    print(f"Linting {len(files)} file(s) with {jobs} job(s)...")
    entries = [(f, args.build_dir, header_filter) for f in files]
    with ProcessPoolExecutor(max_workers=jobs) as pool:
        ok = all(pool.map(run_tidy, entries))
    if not ok:
        sys.exit(1)
    print("clang-tidy: OK")


if __name__ == "__main__":
    main()
