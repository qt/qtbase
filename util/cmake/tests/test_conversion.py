#!/usr/bin/env python3
#############################################################################
##
## Copyright (C) 2022 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the plugins of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

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
    assert(["find_package(Qt6 REQUIRED COMPONENTS Core Network Widgets)"] == find_package_lines)
