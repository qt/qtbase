#!/usr/bin/env python3
# Copyright (C) 2019 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from __future__ import annotations

"""
This utility script shows statistics about
converted .pro -> CMakeLists.txt files.

To execute: python3 pro_conversion_rate.py <src dir>
where <src dir> can be any qt source directory. For better statistics,
specify a module root source dir (like ./qtbase or ./qtsvg).

"""

from argparse import ArgumentParser

import os
import typing
from typing import Dict, Union
from timeit import default_timer


def _parse_commandline():
    parser = ArgumentParser(description="Find pro files for which there are no CMakeLists.txt.")
    parser.add_argument(
        "source_directory", metavar="<src dir>", type=str, help="The source directory"
    )

    return parser.parse_args()


class Blacklist:
    """Class to check if a certain dir_name / dir_path is blacklisted"""

    def __init__(self, names: typing.List[str], path_parts: typing.List[str]):
        self.names = names
        self.path_parts = path_parts

        # The lookup algorithm
        self.lookup = self.is_blacklisted_part
        self.tree = None

        try:
            # If package is available, use Aho-Corasick algorithm,
            from ahocorapy.keywordtree import KeywordTree  # type: ignore

            self.tree = KeywordTree(case_insensitive=True)

            for p in self.path_parts:
                self.tree.add(p)
            self.tree.finalize()

            self.lookup = self.is_blacklisted_part_aho
        except ImportError:
            pass

    def is_blacklisted(self, dir_name: str, dir_path: str) -> bool:
        # First check if exact dir name is blacklisted.
        if dir_name in self.names:
            return True

        # Check if a path part is blacklisted (e.g. util/cmake)
        return self.lookup(dir_path)

    def is_blacklisted_part(self, dir_path: str) -> bool:
        if any(part in dir_path for part in self.path_parts):
            return True
        return False

    def is_blacklisted_part_aho(self, dir_path: str) -> bool:
        return self.tree.search(dir_path) is not None  # type: ignore


def recursive_scan(path: str, extension: str, result_paths: typing.List[str], blacklist: Blacklist):
    """Find files ending with a certain extension, filtering out blacklisted entries"""
    try:
        for entry in os.scandir(path):
            if entry.is_file() and entry.path.endswith(extension):
                result_paths.append(entry.path)
            elif entry.is_dir():
                if blacklist.is_blacklisted(entry.name, entry.path):
                    continue
                recursive_scan(entry.path, extension, result_paths, blacklist)
    except Exception as e:
        print(e)


def check_for_cmake_project(pro_path: str) -> bool:
    pro_dir_name = os.path.dirname(pro_path)
    cmake_project_path = os.path.join(pro_dir_name, "CMakeLists.txt")
    return os.path.exists(cmake_project_path)


def compute_stats(
    src_path: str,
    pros_with_missing_project: typing.List[str],
    total_pros: int,
    existing_pros: int,
    missing_pros: int,
) -> dict:
    stats: Dict[str, Dict[str, Union[str, int, float]]] = {}
    stats["total projects"] = {"label": "Total pro files found", "value": total_pros}
    stats["existing projects"] = {
        "label": "Existing CMakeLists.txt files found",
        "value": existing_pros,
    }
    stats["missing projects"] = {
        "label": "Missing CMakeLists.txt files found",
        "value": missing_pros,
    }
    stats["missing examples"] = {"label": "Missing examples", "value": 0}
    stats["missing tests"] = {"label": "Missing tests", "value": 0}
    stats["missing src"] = {"label": "Missing src/**/**", "value": 0}
    stats["missing plugins"] = {"label": "Missing plugins", "value": 0}

    for p in pros_with_missing_project:
        rel_path = os.path.relpath(p, src_path)
        if rel_path.startswith("examples"):
            assert isinstance(stats["missing examples"]["value"], int)
            stats["missing examples"]["value"] += 1
        elif rel_path.startswith("tests"):
            assert isinstance(stats["missing tests"]["value"], int)
            stats["missing tests"]["value"] += 1
        elif rel_path.startswith(os.path.join("src", "plugins")):
            assert isinstance(stats["missing plugins"]["value"], int)
            stats["missing plugins"]["value"] += 1
        elif rel_path.startswith("src"):
            assert isinstance(stats["missing src"]["value"], int)
            stats["missing src"]["value"] += 1

    for stat in stats:
        if int(stats[stat]["value"]) > 0:
            stats[stat]["percentage"] = round(float(stats[stat]["value"]) * 100 / total_pros, 2)
    return stats


def print_stats(
    src_path: str,
    pros_with_missing_project: typing.List[str],
    stats: dict,
    scan_time: float,
    script_time: float,
):

    if stats["total projects"]["value"] == 0:
        print("No .pro files found. Did you specify a correct source path?")
        return

    if stats["total projects"]["value"] == stats["existing projects"]["value"]:
        print("All projects were converted.")
    else:
        print("Missing CMakeLists.txt files for the following projects: \n")

        for p in pros_with_missing_project:
            rel_path = os.path.relpath(p, src_path)
            print(rel_path)

    print("\nStatistics: \n")

    for stat in stats:
        if stats[stat]["value"] > 0:
            print(
                f"{stats[stat]['label']:<40}: {stats[stat]['value']} ({stats[stat]['percentage']}%)"
            )

    print(f"\n{'Scan time':<40}: {scan_time:.10f} seconds")
    print(f"{'Total script time':<40}: {script_time:.10f} seconds")


def main():
    args = _parse_commandline()
    src_path = os.path.abspath(args.source_directory)
    pro_paths = []

    extension = ".pro"

    blacklist_names = ["config.tests", "doc", "3rdparty", "angle"]
    blacklist_path_parts = [os.path.join("util", "cmake")]

    script_start_time = default_timer()
    blacklist = Blacklist(blacklist_names, blacklist_path_parts)

    scan_time_start = default_timer()
    recursive_scan(src_path, extension, pro_paths, blacklist)
    scan_time_end = default_timer()
    scan_time = scan_time_end - scan_time_start

    total_pros = len(pro_paths)

    pros_with_missing_project = []
    for pro_path in pro_paths:
        if not check_for_cmake_project(pro_path):
            pros_with_missing_project.append(pro_path)

    missing_pros = len(pros_with_missing_project)
    existing_pros = total_pros - missing_pros

    stats = compute_stats(
        src_path, pros_with_missing_project, total_pros, existing_pros, missing_pros
    )
    script_end_time = default_timer()
    script_time = script_end_time - script_start_time

    print_stats(src_path, pros_with_missing_project, stats, scan_time, script_time)


if __name__ == "__main__":
    main()
