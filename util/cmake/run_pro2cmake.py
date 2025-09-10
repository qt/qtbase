#!/usr/bin/env python3
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import glob
import os
import subprocess
import concurrent.futures
import sys
import typing
import argparse
from argparse import ArgumentParser


def parse_command_line() -> argparse.Namespace:
    parser = ArgumentParser(
        description="Run pro2cmake on all .pro files recursively in given path. "
        "You can pass additional arguments to the pro2cmake calls by appending "
        "-- --foo --bar"
    )
    parser.add_argument(
        "--only-existing",
        dest="only_existing",
        action="store_true",
        help="Run pro2cmake only on .pro files that already have a CMakeLists.txt.",
    )
    parser.add_argument(
        "--only-missing",
        dest="only_missing",
        action="store_true",
        help="Run pro2cmake only on .pro files that do not have a CMakeLists.txt.",
    )
    parser.add_argument(
        "--only-qtbase-main-modules",
        dest="only_qtbase_main_modules",
        action="store_true",
        help="Run pro2cmake only on the main modules in qtbase.",
    )
    parser.add_argument(
        "--skip-subdirs-projects",
        dest="skip_subdirs_projects",
        action="store_true",
        help="Don't run pro2cmake on TEMPLATE=subdirs projects.",
    )
    parser.add_argument(
        "--is-example",
        dest="is_example",
        action="store_true",
        help="Run pro2cmake with --is-example flag.",
    )
    parser.add_argument(
        "--count", dest="count", help="How many projects should be converted.", type=int
    )
    parser.add_argument(
        "--offset",
        dest="offset",
        help="From the list of found projects, from which project should conversion begin.",
        type=int,
    )
    parser.add_argument(
        "path", metavar="<path>", type=str, help="The path where to look for .pro files."
    )

    args, unknown = parser.parse_known_args()

    # Error out when the unknown arguments do not start with a "--",
    # which implies passing through arguments to pro2cmake.
    if len(unknown) > 0 and unknown[0] != "--":
        parser.error("unrecognized arguments: {}".format(" ".join(unknown)))
    else:
        args.pro2cmake_args = unknown[1:]

    return args


def find_all_pro_files(base_path: str, args: argparse.Namespace):
    def sorter(pro_file: str) -> str:
        """Sorter that tries to prioritize main pro files in a directory."""
        pro_file_without_suffix = pro_file.rsplit("/", 1)[-1][:-4]
        dir_name = os.path.dirname(pro_file)
        if dir_name == ".":
            dir_name = os.path.basename(os.getcwd())
        if dir_name.endswith(pro_file_without_suffix):
            return dir_name
        return dir_name + "/__" + pro_file

    all_files = []
    previous_dir_name: typing.Optional[str] = None

    print("Finding .pro files.")
    glob_result = glob.glob(os.path.join(base_path, "**/*.pro"), recursive=True)

    def cmake_lists_exists_filter(path):
        path_dir_name = os.path.dirname(path)
        if os.path.exists(os.path.join(path_dir_name, "CMakeLists.txt")):
            return True
        return False

    def cmake_lists_missing_filter(path):
        return not cmake_lists_exists_filter(path)

    def qtbase_main_modules_filter(path):
        main_modules = [
            "corelib",
            "network",
            "gui",
            "widgets",
            "testlib",
            "printsupport",
            "opengl",
            "sql",
            "dbus",
            "concurrent",
            "xml",
        ]
        path_suffixes = [f"src/{m}/{m}.pro" for m in main_modules]

        for path_suffix in path_suffixes:
            if path.endswith(path_suffix):
                return True
        return False

    filter_result = glob_result
    filter_func = None
    if args.only_existing:
        filter_func = cmake_lists_exists_filter
    elif args.only_missing:
        filter_func = cmake_lists_missing_filter
    elif args.only_qtbase_main_modules:
        filter_func = qtbase_main_modules_filter

    if filter_func:
        print("Filtering.")
        filter_result = [p for p in filter_result if filter_func(p)]

    for pro_file in sorted(filter_result, key=sorter):
        dir_name = os.path.dirname(pro_file)
        if dir_name == previous_dir_name:
            print("Skipping:", pro_file)
        else:
            all_files.append(pro_file)
            previous_dir_name = dir_name
    return all_files


def run(all_files: typing.List[str], pro2cmake: str, args: argparse.Namespace) -> typing.List[str]:
    failed_files = []
    files_count = len(all_files)
    workers = os.cpu_count() or 1

    if args.only_qtbase_main_modules:
        # qtbase main modules take longer than usual to process.
        workers = 2

    with concurrent.futures.ThreadPoolExecutor(max_workers=workers, initargs=(10,)) as pool:
        print("Firing up thread pool executor.")

        def _process_a_file(data: typing.Tuple[str, int, int]) -> typing.Tuple[int, str, str]:
            filename, index, total = data
            pro2cmake_args = []
            if sys.platform == "win32":
                pro2cmake_args.append(sys.executable)
            pro2cmake_args.append(pro2cmake)
            if args.is_example:
                pro2cmake_args.append("--is-example")
            if args.skip_subdirs_projects:
                pro2cmake_args.append("--skip-subdirs-project")
            pro2cmake_args.append(os.path.basename(filename))

            if args.pro2cmake_args:
                pro2cmake_args += args.pro2cmake_args

            result = subprocess.run(
                pro2cmake_args,
                cwd=os.path.dirname(filename),
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
            )
            stdout = f"Converted[{index}/{total}]: {filename}\n"
            return result.returncode, filename, stdout + result.stdout.decode()

        for return_code, filename, stdout in pool.map(
            _process_a_file,
            zip(all_files, range(1, files_count + 1), (files_count for _ in all_files)),
        ):
            if return_code:
                failed_files.append(filename)
            print(stdout)

    return failed_files


def main() -> None:
    args = parse_command_line()

    script_path = os.path.dirname(os.path.abspath(__file__))
    pro2cmake = os.path.join(script_path, "pro2cmake.py")
    base_path = args.path

    all_files = find_all_pro_files(base_path, args)
    if args.offset:
        all_files = all_files[args.offset :]
    if args.count:
        all_files = all_files[: args.count]
    files_count = len(all_files)

    failed_files = run(all_files, pro2cmake, args)
    if len(all_files) == 0:
        print("No files found.")

    if failed_files:
        print(
            f"The following files were not successfully "
            f"converted ({len(failed_files)} of {files_count}):"
        )
        for f in failed_files:
            print(f'    "{f}"')


if __name__ == "__main__":
    main()
