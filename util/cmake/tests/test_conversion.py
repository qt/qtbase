#!/usr/bin/env python3
# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from pro2cmake import Scope, SetOperation, merge_scopes, recursive_evaluate_scope
from tempfile import TemporaryDirectory

import os
import pathlib
import pytest
import re
import shutil
import subprocess
import tempfile
import typing

debug_mode = bool(os.environ.get("DEBUG_PRO2CMAKE_TEST_CONVERSION"))
test_script_dir = pathlib.Path(__file__).parent.resolve()
pro2cmake_dir = test_script_dir.parent.resolve()
pro2cmake_py = pro2cmake_dir.joinpath("pro2cmake.py")
test_data_dir = test_script_dir.joinpath("data", "conversion")


def convert(base_name: str):
    pro_file_name = str(base_name) + ".pro"
    pro_file_path = test_data_dir.joinpath(pro_file_name)
    assert(pro_file_path.exists())
    with TemporaryDirectory(prefix="testqmake2cmake") as tmp_dir_str:
        tmp_dir = pathlib.Path(tmp_dir_str)
        output_file_path = tmp_dir.joinpath("CMakeLists.txt")
        exit_code = subprocess.call([pro2cmake_py, "--is-example", "-o", output_file_path, pro_file_path])
        assert(exit_code == 0)
        if debug_mode:
            shutil.copyfile(output_file_path, tempfile.gettempdir() + "/pro2cmake/CMakeLists.txt")
        f = open(output_file_path, "r")
        assert(f)
        content = f.read()
        assert(content)
        return content


def test_qt_modules():
    output = convert("required_qt_modules")
    find_package_lines = []
    for line in output.split("\n"):
        if "find_package(" in line:
            find_package_lines.append(line.strip())
    assert(["find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)",
            "find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network Widgets)"] == find_package_lines)

    output = convert("optional_qt_modules")
    find_package_lines = []
    for line in output.split("\n"):
        if "find_package(" in line:
            find_package_lines.append(line.strip())
    assert(["find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)",
            "find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Network Widgets)",
            "find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS OpenGL)"] == find_package_lines)

def test_qt_version_check():
    output = convert("qt_version_check")
    interesting_lines = []
    for line in output.split("\n"):
        if line.startswith("if(") and "QT_VERSION" in line:
            interesting_lines.append(line.strip())
    assert(["if(( ( (QT_VERSION_MAJOR GREATER 5) ) AND (QT_VERSION_MINOR LESS 1) ) AND (QT_VERSION_PATCH EQUAL 0))", "if(( ( (QT_VERSION VERSION_GREATER 6.6.5) ) AND (QT_VERSION VERSION_LESS 6.6.7) ) AND (QT_VERSION VERSION_EQUAL 6.6.6))"] == interesting_lines)
