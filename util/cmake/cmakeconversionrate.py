#!/usr/bin/env python3
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from argparse import ArgumentParser

import os
import re
import subprocess
import typing


def _parse_commandline():
    parser = ArgumentParser(description="Calculate the conversion rate to cmake.")
    parser.add_argument("--debug", dest="debug", action="store_true", help="Turn on debug output")
    parser.add_argument(
        "source_directory",
        metavar="<Source Directory>",
        type=str,
        help="The Qt module source directory",
    )
    parser.add_argument(
        "binary_directory",
        metavar="<CMake build directory>",
        type=str,
        help="The CMake build directory (might be empty)",
    )

    return parser.parse_args()


def calculate_baseline(source_directory: str, *, debug: bool = False) -> int:
    if debug:
        print(f'Scanning "{source_directory}" for qmake-based tests.')
    result = subprocess.run(
        '/usr/bin/git grep -E "^\\s*CONFIG\\s*\\+?=.*\\btestcase\\b" | sort -u | wc -l',
        shell=True,
        capture_output=True,
        cwd=source_directory,
    )
    return int(result.stdout)


def build(source_directory: str, binary_directory: str, *, debug=False) -> None:
    abs_source = os.path.abspath(source_directory)
    if not os.path.isdir(binary_directory):
        os.makedirs(binary_directory)
    if not os.path.exists(os.path.join(binary_directory, "CMakeCache.txt")):

        if debug:
            print(f'Running cmake in "{binary_directory}"')
        result = subprocess.run(["/usr/bin/cmake", "-GNinja", abs_source], cwd=binary_directory)
        if debug:
            print(f"CMake return code: {result.returncode}.")

        assert result.returncode == 0

    if debug:
        print(f'Running ninja in "{binary_directory}".')
    result = subprocess.run("/usr/bin/ninja", cwd=binary_directory)
    if debug:
        print(f"Ninja return code: {result.returncode}.")

    assert result.returncode == 0


def test(binary_directory: str, *, debug=False) -> typing.Tuple[int, int]:
    if debug:
        print(f'Running ctest in "{binary_directory}".')
    result = subprocess.run(
        '/usr/bin/ctest -j 250 | grep "tests passed, "',
        shell=True,
        capture_output=True,
        cwd=binary_directory,
    )
    summary = result.stdout.decode("utf-8").replace("\n", "")
    if debug:
        print(f"Test summary: {summary} ({result.returncode}).")

    matches = re.fullmatch(r"\d+% tests passed, (\d+) tests failed out of (\d+)", summary)
    if matches:
        if debug:
            print(f"Matches: failed {matches.group(1)}, total {matches.group(2)}.")
        return (int(matches.group(2)), int(matches.group(2)) - int(matches.group(1)))

    return (0, 0)


def main() -> int:
    args = _parse_commandline()

    base_line = calculate_baseline(args.source_directory, debug=args.debug)
    if base_line <= 0:
        print(f"Could not find the qmake baseline in {args.source_directory}.")
        return 1

    if args.debug:
        print(f"qmake baseline: {base_line} test binaries.")

    cmake_total = 0
    cmake_success = 0
    try:
        build(args.source_directory, args.binary_directory, debug=args.debug)
        (cmake_total, cmake_success) = test(args.binary_directory, debug=args.debug)
    finally:
        if cmake_total == 0:
            print("\n\n\nCould not calculate the cmake state.")
            return 2
        else:
            print(f"\n\n\nCMake test conversion rate: {cmake_total/base_line:.2f}.")
            print(f"CMake test success rate   : {cmake_success/base_line:.2f}.")
            return 0


if __name__ == "__main__":
    main()
