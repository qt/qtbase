#!/usr/bin/env python3
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


# Requires Python 3.7. The import statement needs to be the first line of code
# so it's not possible to conditionally check the version and raise an
# exception.
from __future__ import annotations

import copy
import os.path
import posixpath
import sys
import re
import io
import itertools
import glob
import fnmatch

from condition_simplifier import simplify_condition
from condition_simplifier_cache import set_condition_simplified_cache_enabled

import pyparsing as pp  # type: ignore
import xml.etree.ElementTree as ET

from argparse import ArgumentParser
from textwrap import dedent
from functools import lru_cache
from shutil import copyfile
from collections import defaultdict
from typing import (
    List,
    Optional,
    Dict,
    Set,
    IO,
    Union,
    Any,
    Callable,
    FrozenSet,
    Tuple,
    Match,
    Type,
)

from qmake_parser import parseProFile
from special_case_helper import SpecialCaseHandler
from helper import (
    map_qt_library,
    map_3rd_party_library,
    is_known_3rd_party_library,
    featureName,
    map_platform,
    find_library_info_for_target,
    generate_find_package_info,
    LibraryMapping,
)

cmake_version_string = "3.16"
cmake_api_version = 3


def _parse_commandline():
    parser = ArgumentParser(
        description="Generate CMakeLists.txt files from ." "pro files.",
        epilog="Requirements: pip install -r requirements.txt",
    )
    parser.add_argument(
        "--debug", dest="debug", action="store_true", help="Turn on all debug output"
    )
    parser.add_argument(
        "--debug-parser",
        dest="debug_parser",
        action="store_true",
        help="Print debug output from qmake parser.",
    )
    parser.add_argument(
        "--debug-parse-result",
        dest="debug_parse_result",
        action="store_true",
        help="Dump the qmake parser result.",
    )
    parser.add_argument(
        "--debug-parse-dictionary",
        dest="debug_parse_dictionary",
        action="store_true",
        help="Dump the qmake parser result as dictionary.",
    )
    parser.add_argument(
        "--debug-pro-structure",
        dest="debug_pro_structure",
        action="store_true",
        help="Dump the structure of the qmake .pro-file.",
    )
    parser.add_argument(
        "--debug-full-pro-structure",
        dest="debug_full_pro_structure",
        action="store_true",
        help="Dump the full structure of the qmake .pro-file " "(with includes).",
    )
    parser.add_argument(
        "--debug-special-case-preservation",
        dest="debug_special_case_preservation",
        action="store_true",
        help="Show all git commands and file copies.",
    )

    parser.add_argument(
        "--is-example",
        action="store_true",
        dest="is_example",
        help="Treat the input .pro file as a Qt example.",
    )
    parser.add_argument(
        "--is-user-project",
        action="store_true",
        dest="is_user_project",
        help="Treat the input .pro file as a user project.",
    )
    parser.add_argument(
        "-s",
        "--skip-special-case-preservation",
        dest="skip_special_case_preservation",
        action="store_true",
        help="Skips behavior to reapply " "special case modifications (requires git in PATH)",
    )
    parser.add_argument(
        "-k",
        "--keep-temporary-files",
        dest="keep_temporary_files",
        action="store_true",
        help="Don't automatically remove CMakeLists.gen.txt and other " "intermediate files.",
    )

    parser.add_argument(
        "-e",
        "--skip-condition-cache",
        dest="skip_condition_cache",
        action="store_true",
        help="Don't use condition simplifier cache (conversion speed may decrease).",
    )

    parser.add_argument(
        "--skip-subdirs-project",
        dest="skip_subdirs_project",
        action="store_true",
        help="Skip converting project if it ends up being a TEMPLATE=subdirs project.",
    )

    parser.add_argument(
        "-i",
        "--ignore-skip-marker",
        dest="ignore_skip_marker",
        action="store_true",
        help="If set, pro file will be converted even if skip marker is found in CMakeLists.txt.",
    )

    parser.add_argument(
        "--api-version",
        dest="api_version",
        type=int,
        help="Specify which cmake api version should be generated. 1, 2 or 3, 3 is latest.",
    )

    parser.add_argument(
        "-o",
        "--output-file",
        dest="output_file",
        type=str,
        help="Specify a file path where the generated content should be written to. "
        "Default is to write to CMakeLists.txt in the same directory as the .pro file.",
    )

    parser.add_argument(
        "files",
        metavar="<.pro/.pri file>",
        type=str,
        nargs="+",
        help="The .pro/.pri file to process",
    )
    return parser.parse_args()


def get_top_level_repo_project_path(project_file_path: str = "") -> str:
    qmake_or_cmake_conf_path = find_qmake_or_cmake_conf(project_file_path)
    qmake_or_cmake_conf_dir_path = os.path.dirname(qmake_or_cmake_conf_path)
    return qmake_or_cmake_conf_dir_path


def is_top_level_repo_project(project_file_path: str = "") -> bool:
    qmake_or_cmake_conf_path = find_qmake_or_cmake_conf(project_file_path)
    qmake_or_cmake_conf_dir_path = os.path.dirname(qmake_or_cmake_conf_path)
    project_dir_path = os.path.dirname(project_file_path)
    return qmake_or_cmake_conf_dir_path == project_dir_path


def is_top_level_repo_tests_project(project_file_path: str = "") -> bool:
    qmake_or_cmake_conf_path = find_qmake_or_cmake_conf(project_file_path)
    qmake_or_cmake_conf_dir_path = os.path.dirname(qmake_or_cmake_conf_path)
    project_dir_path = os.path.dirname(project_file_path)
    project_dir_name = os.path.basename(project_dir_path)
    maybe_same_level_dir_path = os.path.join(project_dir_path, "..")
    normalized_maybe_same_level_dir_path = os.path.normpath(maybe_same_level_dir_path)
    return (
        qmake_or_cmake_conf_dir_path == normalized_maybe_same_level_dir_path
        and project_dir_name == "tests"
    )


def is_top_level_repo_examples_project(project_file_path: str = "") -> bool:
    qmake_or_cmake_conf_path = find_qmake_or_cmake_conf(project_file_path)
    qmake_or_cmake_conf_dir_path = os.path.dirname(qmake_or_cmake_conf_path)
    project_dir_path = os.path.dirname(project_file_path)
    project_dir_name = os.path.basename(project_dir_path)
    maybe_same_level_dir_path = os.path.join(project_dir_path, "..")
    normalized_maybe_same_level_dir_path = os.path.normpath(maybe_same_level_dir_path)
    return (
        qmake_or_cmake_conf_dir_path == normalized_maybe_same_level_dir_path
        and project_dir_name == "examples"
    )


def is_example_project(project_file_path: str = "") -> bool:
    # If there's a .qmake.conf or .cmake.conf file in the parent
    # directories of the given project path, it is likely that the
    # project is an internal Qt project that uses private Qt CMake
    # API.
    found_qt_repo_version = False
    qmake_conf = find_qmake_conf(project_file_path)
    if qmake_conf:
        repo_version = parse_qt_repo_module_version_from_qmake_conf(qmake_conf)
        if repo_version:
            found_qt_repo_version = True

    cmake_conf = find_cmake_conf(project_file_path)
    if cmake_conf:
        repo_version = parse_qt_repo_module_version_from_cmake_conf(cmake_conf)
        if repo_version:
            found_qt_repo_version = True

    # If we haven't found a conf file, we assume this is an example
    # project and not a project under a qt source repository.
    if not found_qt_repo_version:
        return True

    # If the project file is found in a subdir called 'examples'
    # relative to the repo source dir, then it must be an example, but
    # some examples contain 3rdparty libraries that do not need to be
    # built as examples.
    qmake_or_cmake_conf_path = find_qmake_or_cmake_conf(project_file_path)
    qmake_or_cmake_conf_dir_path = os.path.dirname(qmake_or_cmake_conf_path)
    project_relative_path = os.path.relpath(project_file_path, qmake_or_cmake_conf_dir_path)

    is_example_under_repo_sources = (
        project_relative_path.startswith("examples") and "3rdparty" not in project_relative_path
    )
    return is_example_under_repo_sources


def is_config_test_project(project_file_path: str = "") -> bool:
    qmake_or_cmake_conf_path = find_qmake_or_cmake_conf(project_file_path)
    qmake_or_cmake_conf_dir_path = os.path.dirname(qmake_or_cmake_conf_path)
    dir_name_with_qmake_or_cmake_conf = os.path.basename(qmake_or_cmake_conf_dir_path)

    project_relative_path = os.path.relpath(project_file_path, qmake_or_cmake_conf_dir_path)
    # If the project file is found in a subdir called 'config.tests'
    # relative to the repo source dir, then it's probably a config test.
    # Also if the .qmake.conf is found within config.tests dir (like in qtbase)
    # then the project is probably a config .test
    return (
        project_relative_path.startswith("config.tests")
        or dir_name_with_qmake_or_cmake_conf == "config.tests"
    )


def is_benchmark_project(project_file_path: str = "") -> bool:
    qmake_or_cmake_conf_path = find_qmake_or_cmake_conf(project_file_path)
    qmake_or_cmake_conf_dir_path = os.path.dirname(qmake_or_cmake_conf_path)

    project_relative_path = os.path.relpath(project_file_path, qmake_or_cmake_conf_dir_path)
    # If the project file is found in a subdir called 'tests/benchmarks'
    # relative to the repo source dir, then it must be a benchmark
    return project_relative_path.startswith("tests/benchmarks")


def is_manual_test_project(project_file_path: str = "") -> bool:
    qmake_or_cmake_conf_path = find_qmake_or_cmake_conf(project_file_path)
    qmake_or_cmake_conf_dir_path = os.path.dirname(qmake_or_cmake_conf_path)

    project_relative_path = os.path.relpath(project_file_path, qmake_or_cmake_conf_dir_path)
    # If the project file is found in a subdir called 'tests/manual'
    # relative to the repo source dir, then it must be a manual test
    return project_relative_path.startswith("tests/manual")


@lru_cache(maxsize=None)
def find_file_walking_parent_dirs(file_name: str, project_file_path: str = "") -> str:
    assert file_name
    if not os.path.isabs(project_file_path):
        print(
            f"Warning: could not find {file_name} file, given path is not an "
            f"absolute path: {project_file_path}"
        )
        return ""

    cwd = os.path.dirname(project_file_path)

    while os.path.isdir(cwd):
        maybe_file = posixpath.join(cwd, file_name)
        if os.path.isfile(maybe_file):
            return maybe_file
        else:
            last_cwd = cwd
            cwd = os.path.dirname(cwd)
            if last_cwd == cwd:
                # reached the top level directory, stop looking
                break

    return ""


def find_qmake_conf(project_file_path: str = "") -> str:
    return find_file_walking_parent_dirs(".qmake.conf", project_file_path)


def find_cmake_conf(project_file_path: str = "") -> str:
    return find_file_walking_parent_dirs(".cmake.conf", project_file_path)


def find_qmake_or_cmake_conf(project_file_path: str = "") -> str:
    qmake_conf = find_qmake_conf(project_file_path)
    if qmake_conf:
        return qmake_conf
    cmake_conf = find_cmake_conf(project_file_path)
    return cmake_conf


def parse_qt_repo_module_version_from_qmake_conf(qmake_conf_path: str = "") -> str:
    with open(qmake_conf_path) as f:
        file_contents = f.read()
        m = re.search(r"MODULE_VERSION\s*=\s*([0-9.]+)", file_contents)
    return m.group(1) if m else ""


def parse_qt_repo_module_version_from_cmake_conf(cmake_conf_path: str = "") -> str:
    with open(cmake_conf_path) as f:
        file_contents = f.read()
        m = re.search(r'set\(QT_REPO_MODULE_VERSION\s*"([0-9.]+)"\)', file_contents)
    return m.group(1) if m else ""


def set_up_cmake_api_calls():
    def nested_dict():
        return defaultdict(nested_dict)

    api = nested_dict()

    api[1]["qt_extend_target"] = "extend_target"
    api[1]["qt_add_module"] = "add_qt_module"
    api[1]["qt_add_plugin"] = "add_qt_plugin"
    api[1]["qt_add_tool"] = "add_qt_tool"
    api[2]["qt_internal_add_app"] = "qt_internal_add_app"
    api[1]["qt_add_test"] = "add_qt_test"
    api[1]["qt_add_test_helper"] = "add_qt_test_helper"
    api[1]["qt_add_manual_test"] = "add_qt_manual_test"
    api[1]["qt_add_benchmark"] = "add_qt_benchmark"
    api[1]["qt_add_executable"] = "add_qt_executable"
    api[1]["qt_add_simd_part"] = "add_qt_simd_part"
    api[1]["qt_add_docs"] = "add_qt_docs"
    api[1]["qt_add_resource"] = "add_qt_resource"
    api[1]["qt_add_qml_module"] = "add_qml_module"
    api[1]["qt_add_cmake_library"] = "add_cmake_library"
    api[1]["qt_add_3rdparty_library"] = "qt_add_3rdparty_library"
    api[1]["qt_create_tracepoints"] = "qt_create_tracepoints"

    api[2]["qt_extend_target"] = "qt_extend_target"
    api[2]["qt_add_module"] = "qt_add_module"
    api[2]["qt_add_plugin"] = "qt_internal_add_plugin"
    api[2]["qt_add_tool"] = "qt_add_tool"
    api[2]["qt_internal_add_app"] = "qt_internal_add_app"
    api[2]["qt_add_test"] = "qt_add_test"
    api[2]["qt_add_test_helper"] = "qt_add_test_helper"
    api[2]["qt_add_manual_test"] = "qt_add_manual_test"
    api[2]["qt_add_benchmark"] = "qt_add_benchmark"
    api[2]["qt_add_executable"] = "qt_add_executable"
    api[2]["qt_add_simd_part"] = "qt_add_simd_part"
    api[2]["qt_add_docs"] = "qt_add_docs"
    api[2]["qt_add_resource"] = "qt_add_resource"
    api[2]["qt_add_qml_module"] = "qt_add_qml_module"
    api[2]["qt_add_cmake_library"] = "qt_add_cmake_library"
    api[2]["qt_add_3rdparty_library"] = "qt_add_3rdparty_library"
    api[2]["qt_create_tracepoints"] = "qt_create_tracepoints"

    api[3]["qt_extend_target"] = "qt_internal_extend_target"
    api[3]["qt_add_module"] = "qt_internal_add_module"
    api[3]["qt_add_plugin"] = "qt_internal_add_plugin"
    api[3]["qt_add_tool"] = "qt_internal_add_tool"
    api[3]["qt_internal_add_app"] = "qt_internal_add_app"
    api[3]["qt_add_test"] = "qt_internal_add_test"
    api[3]["qt_add_test_helper"] = "qt_internal_add_test_helper"
    api[3]["qt_add_manual_test"] = "qt_internal_add_manual_test"
    api[3]["qt_add_benchmark"] = "qt_internal_add_benchmark"
    api[3]["qt_add_executable"] = "qt_internal_add_executable"
    api[3]["qt_add_simd_part"] = "qt_internal_add_simd_part"
    api[3]["qt_add_docs"] = "qt_internal_add_docs"
    api[3]["qt_add_resource"] = "qt_internal_add_resource"
    api[3]["qt_add_qml_module"] = "qt_internal_add_qml_module"
    api[3]["qt_add_cmake_library"] = "qt_internal_add_cmake_library"
    api[3]["qt_add_3rdparty_library"] = "qt_internal_add_3rdparty_library"
    api[3]["qt_create_tracepoints"] = "qt_internal_create_tracepoints"

    return api


cmake_api_calls = set_up_cmake_api_calls()


def detect_cmake_api_version_used_in_file_content(project_file_path: str) -> Optional[int]:
    dir_path = os.path.dirname(project_file_path)
    cmake_project_path = os.path.join(dir_path, "CMakeLists.txt")

    # If file doesn't exist, None implies default version selected by
    # script.
    if not os.path.exists(cmake_project_path):
        return None

    with open(cmake_project_path, "r") as file_fd:
        contents = file_fd.read()

        api_call_versions = [version for version in cmake_api_calls]
        api_call_versions = sorted(api_call_versions, reverse=True)
        api_call_version_matches = {}
        for version in api_call_versions:
            versioned_api_calls = [
                cmake_api_calls[version][api_call] for api_call in cmake_api_calls[version]
            ]
            versioned_api_calls_alternatives = "|".join(versioned_api_calls)
            api_call_version_matches[version] = re.search(
                versioned_api_calls_alternatives, contents
            )

        # If new style found, return latest api version. Otherwise
        # return the current version.
        for version in api_call_version_matches:
            if api_call_version_matches[version]:
                return version

    return 1


def get_cmake_api_call(api_name: str, api_version: Optional[int] = None) -> str:
    if not api_version:
        global cmake_api_version
        api_version = cmake_api_version
    if not cmake_api_calls[api_version][api_name]:
        raise RuntimeError(f"No CMake API call {api_name} of version {api_version} found.")

    return cmake_api_calls[api_version][api_name]


class QtResource:
    def __init__(
        self,
        name: str = "",
        prefix: str = "",
        base_dir: str = "",
        files: Dict[str, str] = {},
        lang: str = None,
        generated: bool = False,
        skip_qtquick_compiler: bool = False,
    ) -> None:
        self.name = name
        self.prefix = prefix
        self.base_dir = base_dir
        self.files = files
        self.lang = lang
        self.generated = generated
        self.skip_qtquick_compiler = skip_qtquick_compiler


def read_qrc_file(
    filepath: str,
    base_dir: str = "",
    project_file_path: str = "",
    skip_qtquick_compiler: bool = False,
) -> List[QtResource]:
    # Hack to handle QT_SOURCE_TREE. Assume currently that it's the same
    # as the qtbase source path.
    qt_source_tree_literal = "${QT_SOURCE_TREE}"
    if qt_source_tree_literal in filepath:
        qmake_or_cmake_conf = find_qmake_or_cmake_conf(project_file_path)

        if qmake_or_cmake_conf:
            qt_source_tree = os.path.dirname(qmake_or_cmake_conf)
            filepath = filepath.replace(qt_source_tree_literal, qt_source_tree)
        else:
            print(
                f"Warning, could not determine QT_SOURCE_TREE location while trying "
                f"to find: {filepath}"
            )

    resource_name = os.path.splitext(os.path.basename(filepath))[0]
    dir_name = os.path.dirname(filepath)
    base_dir = posixpath.join("" if base_dir == "." else base_dir, dir_name)

    # Small not very thorough check to see if this a shared qrc resource
    # pattern is mostly used by the tests.
    if not os.path.isfile(filepath):
        raise RuntimeError(f"Invalid file path given to process_qrc_file: {filepath}")

    tree = ET.parse(filepath)
    root = tree.getroot()
    assert root.tag == "RCC"

    result: List[QtResource] = []
    for resource in root:
        assert resource.tag == "qresource"
        r = QtResource(
            name=resource_name,
            prefix=resource.get("prefix", "/"),
            base_dir=base_dir,
            lang=resource.get("lang", ""),
            skip_qtquick_compiler=skip_qtquick_compiler,
        )

        if len(result) > 0:
            r.name += str(len(result))

        if not r.prefix.startswith("/"):
            r.prefix = f"/{r.prefix}"

        for file in resource:
            path = file.text
            assert path

            # Get alias:
            alias = file.get("alias", "")
            r.files[path] = alias

        result.append(r)

    return result


def write_resource_source_file_properties(
    sorted_files: List[str], files: Dict[str, str], base_dir: str, skip_qtquick_compiler: bool
) -> str:
    output = ""
    source_file_properties = defaultdict(list)

    for source in sorted_files:
        alias = files[source]
        if alias:
            source_file_properties[source].append(f'QT_RESOURCE_ALIAS "{alias}"')
        # If a base dir is given, we have to write the source file property
        # assignments that disable the quick compiler per file.
        if base_dir and skip_qtquick_compiler:
            source_file_properties[source].append("QT_SKIP_QUICKCOMPILER 1")

    for full_source in source_file_properties:
        per_file_props = source_file_properties[full_source]
        if per_file_props:
            prop_spaces = "                                "
            per_file_props_joined = f"\n{prop_spaces}".join(per_file_props)
            output += dedent(
                f"""\
                            set_source_files_properties("{full_source}"
                                PROPERTIES {per_file_props_joined}
                            )
                        """
            )

    return output


def write_add_qt_resource_call(
    target: str,
    scope: Scope,
    resource_name: str,
    prefix: Optional[str],
    base_dir: str,
    lang: Optional[str],
    files: Dict[str, str],
    skip_qtquick_compiler: bool,
    is_example: bool,
) -> str:
    output = ""

    if base_dir:
        base_dir_expanded = scope.expandString(base_dir)
        if base_dir_expanded:
            base_dir = base_dir_expanded
        new_files = {}
        for file_path, alias in files.items():
            full_file_path = posixpath.join(base_dir, file_path)
            new_files[full_file_path] = alias
        files = new_files

    sorted_files = sorted(files.keys())
    assert sorted_files

    output += write_resource_source_file_properties(
        sorted_files, files, base_dir, skip_qtquick_compiler
    )

    # Quote file paths in case there are spaces.
    sorted_files_backup = sorted_files
    sorted_files = []
    for source in sorted_files_backup:
        if source.startswith("${"):
            sorted_files.append(source)
        else:
            sorted_files.append(f'"{source}"')

    file_list = "\n            ".join(sorted_files)
    output += dedent(
        f"""\
        set({resource_name}_resource_files
            {file_list}
        )\n
        """
    )
    file_list = f"${{{resource_name}_resource_files}}"
    if skip_qtquick_compiler and not base_dir:
        output += (
            f"set_source_files_properties(${{{resource_name}_resource_files}}"
            " PROPERTIES QT_SKIP_QUICKCOMPILER 1)\n\n"
        )

    prefix_expanded = scope.expandString(str(prefix))
    if prefix_expanded:
        prefix = prefix_expanded
    params = ""
    if lang:
        params += f'{spaces(1)}LANG\n{spaces(2)}"{lang}"\n'
    params += f'{spaces(1)}PREFIX\n{spaces(2)}"{prefix}"\n'
    if base_dir:
        params += f'{spaces(1)}BASE\n{spaces(2)}"{base_dir}"\n'
    if is_example:
        add_resource_command = "qt6_add_resources"
    else:
        add_resource_command = get_cmake_api_call("qt_add_resource")
    output += (
        f'{add_resource_command}({target} "{resource_name}"\n{params}{spaces(1)}FILES\n'
        f"{spaces(2)}{file_list}\n)\n"
    )

    return output


class QmlDirFileInfo:
    def __init__(self, file_path: str, type_name: str) -> None:
        self.file_path = file_path
        self.versions = ""
        self.type_name = type_name
        self.internal = False
        self.singleton = False
        self.path = ""


class QmlDir:
    def __init__(self) -> None:
        self.module = ""
        self.plugin_name = ""
        self.plugin_optional = False
        self.plugin_path = ""
        self.classname = ""
        self.imports: List[str] = []
        self.optional_imports: List[str] = []
        self.type_names: Dict[str, QmlDirFileInfo] = {}
        self.type_infos: List[str] = []
        self.depends: List[Tuple[str, str]] = []
        self.designer_supported = False

    def __str__(self) -> str:
        type_infos_line = "    \n".join(self.type_infos)
        imports_line = "    \n".join(self.imports)
        optional_imports_line = "    \n".join(self.optional_imports)
        string = f"""\
            module: {self.module}
            plugin: {self.plugin_optional} {self.plugin_name} {self.plugin_path}
            classname: {self.classname}
            type_infos:{type_infos_line}
            imports:{imports_line}
            optional_imports:{optional_imports_line}
            dependends:
            """
        for dep in self.depends:
            string += f"    {dep[0]} {dep[1]}\n"
        string += f"designer supported: {self.designer_supported}\n"
        string += "type_names:\n"
        for key in self.type_names:
            file_info = self.type_names[key]
            string += (
                f"    type:{file_info.type_name} "
                f"versions:{file_info.versions} "
                f"path:{file_info.file_path} "
                f"internal:{file_info.internal} "
                f"singleton:{file_info.singleton}\n"
            )
        return string

    def get_or_create_file_info(self, path: str, type_name: str) -> QmlDirFileInfo:
        if path not in self.type_names:
            self.type_names[path] = QmlDirFileInfo(path, type_name)
        qmldir_file = self.type_names[path]
        if qmldir_file.type_name != type_name:
            raise RuntimeError("Registered qmldir file type_name does not match.")
        return qmldir_file

    def handle_file_internal(self, type_name: str, path: str):
        qmldir_file = self.get_or_create_file_info(path, type_name)
        qmldir_file.internal = True

    def handle_file_singleton(self, type_name: str, version: str, path: str):
        qmldir_file = self.handle_file(type_name, version, path)
        qmldir_file.singleton = True

    def handle_file(self, type_name: str, version: str, path: str) -> QmlDirFileInfo:
        qmldir_file = self.get_or_create_file_info(path, type_name)
        # If this is not the first version we've found,
        # append ';' to delineate the next version; e.g.: "2.0;2.6"
        if qmldir_file.versions:
            qmldir_file.versions += ";"
        qmldir_file.versions += version
        qmldir_file.type_name = type_name
        qmldir_file.path = path
        return qmldir_file

    def from_lines(self, lines: List[str]):
        for line in lines:
            self.handle_line(line)

    def from_file(self, path: str):
        f = open(path, "r")
        if not f:
            raise RuntimeError(f"Failed to open qmldir file at: {path}")
        for line in f:
            self.handle_line(line)

    def handle_line(self, line: str):
        if line.startswith("#"):
            return
        line = line.strip().replace("\n", "")
        if len(line) == 0:
            return

        entries = line.split(" ")
        if len(entries) == 0:
            raise RuntimeError("Unexpected QmlDir file line entry")
        if entries[0] == "module":
            self.module = entries[1]
        elif entries[0] == "singleton":
            self.handle_file_singleton(entries[1], entries[2], entries[3])
        elif entries[0] == "internal":
            self.handle_file_internal(entries[1], entries[2])
        elif entries[0] == "plugin":
            self.plugin_name = entries[1]
            if len(entries) > 2:
                self.plugin_path = entries[2]
        elif entries[0] == "optional":
            if entries[1] == "plugin":
                self.plugin_name = entries[2]
                self.plugin_optional = True
                if len(entries) > 3:
                    self.plugin_path = entries[3]
            elif entries[1] == "import":
                if len(entries) == 4:
                    self.optional_imports.append(entries[2] + "/" + entries[3])
                else:
                    self.optional_imports.append(entries[2])
            else:
                raise RuntimeError("Only plugins and imports can be optional in qmldir files")
        elif entries[0] == "classname":
            self.classname = entries[1]
        elif entries[0] == "typeinfo":
            self.type_infos.append(entries[1])
        elif entries[0] == "depends":
            self.depends.append((entries[1], entries[2]))
        elif entries[0] == "designersupported":
            self.designer_supported = True
        elif entries[0] == "import":
            if len(entries) == 3:
                self.imports.append(entries[1] + "/" + entries[2])
            else:
                self.imports.append(entries[1])
        elif len(entries) == 3:
            self.handle_file(entries[0], entries[1], entries[2])
        else:
            raise RuntimeError(f"Uhandled qmldir entry {line}")


def spaces(indent: int) -> str:
    return "    " * indent


def trim_leading_dot(file: str) -> str:
    while file.startswith("./"):
        file = file[2:]
    return file


def map_to_file(f: str, scope: Scope, *, is_include: bool = False) -> str:
    assert "$$" not in f

    if f.startswith("${"):  # Some cmake variable is prepended
        return f

    base_dir = scope.currentdir if is_include else scope.basedir
    f = posixpath.join(base_dir, f)

    return trim_leading_dot(f)


def handle_vpath(source: str, base_dir: str, vpath: List[str]) -> str:
    assert "$$" not in source

    if not source:
        return ""

    if not vpath:
        return source

    if os.path.exists(os.path.join(base_dir, source)):
        return source

    variable_pattern = re.compile(r"\$\{[A-Za-z0-9_]+\}")
    match = re.match(variable_pattern, source)
    if match:
        # a complex, variable based path, skipping validation
        # or resolving
        return source

    for v in vpath:
        fullpath = posixpath.join(v, source)
        if os.path.exists(fullpath):
            return trim_leading_dot(posixpath.relpath(fullpath, base_dir))

    print(f"    XXXX: Source {source}: Not found.")
    return f"{source}-NOTFOUND"


class Operation:
    def __init__(self, value: Union[List[str], str], line_no: int = -1) -> None:
        if isinstance(value, list):
            self._value = value
        else:
            self._value = [str(value)]
        self._line_no = line_no

    def process(
        self, key: str, sinput: List[str], transformer: Callable[[List[str]], List[str]]
    ) -> List[str]:
        assert False

    def __repr__(self):
        assert False

    def _dump(self):
        if not self._value:
            return "<NOTHING>"

        if not isinstance(self._value, list):
            return "<NOT A LIST>"

        result = []
        for i in self._value:
            if not i:
                result.append("<NONE>")
            else:
                result.append(str(i))
        return '"' + '", "'.join(result) + '"'


class AddOperation(Operation):
    def process(
        self, key: str, sinput: List[str], transformer: Callable[[List[str]], List[str]]
    ) -> List[str]:
        return sinput + transformer(self._value)

    def __repr__(self):
        return f"+({self._dump()})"


class UniqueAddOperation(Operation):
    def process(
        self, key: str, sinput: List[str], transformer: Callable[[List[str]], List[str]]
    ) -> List[str]:
        result = sinput
        for v in transformer(self._value):
            if v not in result:
                result.append(v)
        return result

    def __repr__(self):
        return f"*({self._dump()})"


class ReplaceOperation(Operation):
    def process(
        self, key: str, sinput: List[str], transformer: Callable[[List[str]], List[str]]
    ) -> List[str]:
        result = []
        for s in sinput:
            for v in transformer(self._value):
                pattern, replacement = self.split_rex(v)
                result.append(re.sub(pattern, replacement, s))
        return result

    def split_rex(self, s):
        pattern = ""
        replacement = ""
        if len(s) < 4:
            return pattern, replacement
        sep = s[1]
        s = s[2:]
        rex = re.compile(f"[^\\\\]{sep}")
        m = rex.search(s)
        if not m:
            return pattern, replacement
        pattern = s[: m.start() + 1]
        replacement = s[m.end() :]
        m = rex.search(replacement)
        if m:
            replacement = replacement[: m.start() + 1]
        return pattern, replacement

    def __repr__(self):
        return f"*({self._dump()})"


class SetOperation(Operation):
    def process(
        self, key: str, sinput: List[str], transformer: Callable[[List[str]], List[str]]
    ) -> List[str]:
        values = []  # List[str]
        for v in self._value:
            if v != f"$${key}":
                values.append(v)
            else:
                values += sinput

        if transformer:
            return list(transformer(values))
        else:
            return values

    def __repr__(self):
        return f"=({self._dump()})"


class RemoveOperation(Operation):
    def process(
        self, key: str, sinput: List[str], transformer: Callable[[List[str]], List[str]]
    ) -> List[str]:
        sinput_set = set(sinput)
        value_set = set(self._value)
        result: List[str] = []

        # Add everything that is not going to get removed:
        for v in sinput:
            if v not in value_set:
                result += [v]

        # Add everything else with removal marker:
        for v in transformer(self._value):
            if v not in sinput_set:
                result += [f"-{v}"]

        return result

    def __repr__(self):
        return f"-({self._dump()})"


# Helper class that stores a list of tuples, representing a scope id and
# a line number within that scope's project file. The whole list
# represents the full path location for a certain operation while
# traversing include()'d scopes. Used for sorting when determining
# operation order when evaluating operations.
class OperationLocation(object):
    def __init__(self):
        self.list_of_scope_ids_and_line_numbers = []

    def clone_and_append(self, scope_id: int, line_number: int) -> OperationLocation:
        new_location = OperationLocation()
        new_location.list_of_scope_ids_and_line_numbers = list(
            self.list_of_scope_ids_and_line_numbers
        )
        new_location.list_of_scope_ids_and_line_numbers.append((scope_id, line_number))
        return new_location

    def __lt__(self, other: OperationLocation) -> Any:
        return self.list_of_scope_ids_and_line_numbers < other.list_of_scope_ids_and_line_numbers

    def __repr__(self) -> str:
        s = ""
        for t in self.list_of_scope_ids_and_line_numbers:
            s += f"s{t[0]}:{t[1]} "
        s = s.strip(" ")
        return s


class Scope(object):

    SCOPE_ID: int = 1

    def __init__(
        self,
        *,
        parent_scope: Optional[Scope],
        qmake_file: str,
        condition: str = "",
        base_dir: str = "",
        operations: Union[Dict[str, List[Operation]], None] = None,
        parent_include_line_no: int = -1,
    ) -> None:
        if not operations:
            operations = {
                "QT_SOURCE_TREE": [SetOperation(["${QT_SOURCE_TREE}"])],
                "QT_BUILD_TREE": [SetOperation(["${PROJECT_BINARY_DIR}"])],
                "QTRO_SOURCE_TREE": [SetOperation(["${CMAKE_SOURCE_DIR}"])],
            }

        self._operations: Dict[str, List[Operation]] = copy.deepcopy(operations)
        if parent_scope:
            parent_scope._add_child(self)
        else:
            self._parent = None  # type: Optional[Scope]
            # Only add the  "QT = core gui" Set operation once, on the
            # very top-level .pro scope, aka it's basedir is empty.
            if not base_dir:
                self._operations["QT"] = [SetOperation(["core", "gui"])]

        self._basedir = base_dir
        if qmake_file:
            self._currentdir = os.path.dirname(qmake_file) or "."
        if not self._basedir:
            self._basedir = self._currentdir

        self._scope_id = Scope.SCOPE_ID
        Scope.SCOPE_ID += 1
        self._file = qmake_file
        self._file_absolute_path = os.path.abspath(qmake_file)
        self._condition = map_condition(condition)
        self._children = []  # type: List[Scope]
        self._included_children = []  # type: List[Scope]
        self._including_scope = None  # type: Optional[Scope]
        self._visited_keys = set()  # type: Set[str]
        self._total_condition = None  # type: Optional[str]
        self._parent_include_line_no = parent_include_line_no
        self._is_public_module = False
        self._has_private_module = False
        self._is_internal_qt_app = False

    def __repr__(self):
        return (
            f"{self._scope_id}:{self._basedir}:{self._currentdir}:{self._file}:"
            f"{self._condition or '<TRUE>'}"
        )

    def reset_visited_keys(self):
        self._visited_keys = set()

    def merge(self, other: "Scope") -> None:
        assert self != other
        other._including_scope = self
        self._included_children.append(other)

    @property
    def scope_debug(self) -> bool:
        merge = self.get_string("PRO2CMAKE_SCOPE_DEBUG").lower()
        return merge == "1" or merge == "on" or merge == "yes" or merge == "true"

    @property
    def parent(self) -> Optional[Scope]:
        return self._parent

    @property
    def including_scope(self) -> Optional[Scope]:
        return self._including_scope

    @property
    def basedir(self) -> str:
        return self._basedir

    @property
    def currentdir(self) -> str:
        return self._currentdir

    @property
    def is_public_module(self) -> bool:
        return self._is_public_module

    @property
    def has_private_module(self) -> bool:
        return self._has_private_module

    @property
    def is_internal_qt_app(self) -> bool:
        is_app = self._is_internal_qt_app
        current_scope = self
        while not is_app and current_scope.parent:
            current_scope = current_scope.parent
            is_app = current_scope.is_internal_qt_app

        return is_app

    def can_merge_condition(self):
        if self._condition == "else":
            return False
        if self._operations:
            return False

        child_count = len(self._children)
        if child_count == 0 or child_count > 2:
            return False
        assert child_count != 1 or self._children[0]._condition != "else"
        return child_count == 1 or self._children[1]._condition == "else"

    def settle_condition(self):
        new_children: List[Scope] = []
        for c in self._children:
            c.settle_condition()

            if c.can_merge_condition():
                child = c._children[0]
                child._condition = "({c._condition}) AND ({child._condition})"
                new_children += c._children
            else:
                new_children.append(c)
        self._children = new_children

    @staticmethod
    def FromDict(
        parent_scope: Optional["Scope"],
        file: str,
        statements,
        cond: str = "",
        base_dir: str = "",
        project_file_content: str = "",
        parent_include_line_no: int = -1,
    ) -> Scope:
        scope = Scope(
            parent_scope=parent_scope,
            qmake_file=file,
            condition=cond,
            base_dir=base_dir,
            parent_include_line_no=parent_include_line_no,
        )
        for statement in statements:
            if isinstance(statement, list):  # Handle skipped parts...
                assert not statement
                continue

            operation = statement.get("operation", None)
            if operation:
                key = statement.get("key", "")
                value = statement.get("value", [])
                assert key != ""

                op_location_start = operation["locn_start"]
                operation = operation["value"]
                op_line_no = pp.lineno(op_location_start, project_file_content)

                if operation == "=":
                    scope._append_operation(key, SetOperation(value, line_no=op_line_no))
                elif operation == "-=":
                    scope._append_operation(key, RemoveOperation(value, line_no=op_line_no))
                elif operation == "+=":
                    scope._append_operation(key, AddOperation(value, line_no=op_line_no))
                elif operation == "*=":
                    scope._append_operation(key, UniqueAddOperation(value, line_no=op_line_no))
                elif operation == "~=":
                    scope._append_operation(key, ReplaceOperation(value, line_no=op_line_no))
                else:
                    print(f'Unexpected operation "{operation}" in scope "{scope}".')
                    assert False

                continue

            condition = statement.get("condition", None)
            if condition:
                Scope.FromDict(scope, file, statement.get("statements"), condition, scope.basedir)

                else_statements = statement.get("else_statements")
                if else_statements:
                    Scope.FromDict(scope, file, else_statements, "else", scope.basedir)
                continue

            loaded = statement.get("loaded")
            if loaded:
                scope._append_operation("_LOADED", UniqueAddOperation(loaded))
                continue

            option = statement.get("option", None)
            if option:
                scope._append_operation("_OPTION", UniqueAddOperation(option))
                continue

            included = statement.get("included", None)
            if included:
                included_location_start = included["locn_start"]
                included = included["value"]
                included_line_no = pp.lineno(included_location_start, project_file_content)
                scope._append_operation(
                    "_INCLUDED", UniqueAddOperation(included, line_no=included_line_no)
                )
                continue

            project_required_condition = statement.get("project_required_condition")
            if project_required_condition:
                scope._append_operation("_REQUIREMENTS", AddOperation(project_required_condition))

            qt_no_make_tools = statement.get("qt_no_make_tools_arguments")
            if qt_no_make_tools:
                qt_no_make_tools = qt_no_make_tools.strip("()").strip()
                qt_no_make_tools = qt_no_make_tools.split()
                for entry in qt_no_make_tools:
                    scope._append_operation("_QT_NO_MAKE_TOOLS", AddOperation(entry))

        scope.settle_condition()

        if scope.scope_debug:
            print(f"..... [SCOPE_DEBUG]: Created scope {scope}:")
            scope.dump(indent=1)
            print("..... [SCOPE_DEBUG]: <<END OF SCOPE>>")
        return scope

    def _append_operation(self, key: str, op: Operation) -> None:
        if key in self._operations:
            self._operations[key].append(op)
        else:
            self._operations[key] = [op]

    @property
    def file(self) -> str:
        return self._file or ""

    @property
    def file_absolute_path(self) -> str:
        return self._file_absolute_path or ""

    @property
    def generated_cmake_lists_path(self) -> str:
        assert self.basedir
        return os.path.join(self.basedir, "CMakeLists.gen.txt")

    @property
    def original_cmake_lists_path(self) -> str:
        assert self.basedir
        return os.path.join(self.basedir, "CMakeLists.txt")

    @property
    def condition(self) -> str:
        return self._condition

    @property
    def total_condition(self) -> Optional[str]:
        return self._total_condition

    @total_condition.setter
    def total_condition(self, condition: str) -> None:
        self._total_condition = condition

    def _add_child(self, scope: "Scope") -> None:
        scope._parent = self
        self._children.append(scope)

    @property
    def children(self) -> List["Scope"]:
        result = list(self._children)
        for include_scope in self._included_children:
            result += include_scope.children
        return result

    def dump(self, *, indent: int = 0) -> None:
        ind = spaces(indent)
        print(f'{ind}Scope "{self}":')
        if self.total_condition:
            print(f"{ind}  Total condition = {self.total_condition}")
        print(f"{ind}  Keys:")
        keys = self._operations.keys()
        if not keys:
            print(f"{ind}    -- NONE --")
        else:
            for k in sorted(keys):
                print(f'{ind}    {k} = "{self._operations.get(k, [])}"')
        print(f"{ind}  Children:")
        if not self._children:
            print(f"{ind}    -- NONE --")
        else:
            for c in self._children:
                c.dump(indent=indent + 1)
        print(f"{ind}  Includes:")
        if not self._included_children:
            print(f"{ind}    -- NONE --")
        else:
            for c in self._included_children:
                c.dump(indent=indent + 1)

    def dump_structure(self, *, structure_type: str = "ROOT", indent: int = 0) -> None:
        print(f"{spaces(indent)}{structure_type}: {self}")
        for i in self._included_children:
            i.dump_structure(structure_type="INCL", indent=indent + 1)
        for i in self._children:
            i.dump_structure(structure_type="CHLD", indent=indent + 1)

    @property
    def keys(self):
        return self._operations.keys()

    @property
    def visited_keys(self):
        return self._visited_keys

    # Traverses a scope and its children, and collects operations
    # that need to be processed for a certain key.
    def _gather_operations_from_scope(
        self,
        operations_result: List[Dict[str, Any]],
        current_scope: Scope,
        op_key: str,
        current_location: OperationLocation,
    ):
        for op in current_scope._operations.get(op_key, []):
            new_op_location = current_location.clone_and_append(
                current_scope._scope_id, op._line_no
            )
            op_info: Dict[str, Any] = {}
            op_info["op"] = op
            op_info["scope"] = current_scope
            op_info["location"] = new_op_location
            operations_result.append(op_info)

        for included_child in current_scope._included_children:
            new_scope_location = current_location.clone_and_append(
                current_scope._scope_id, included_child._parent_include_line_no
            )
            self._gather_operations_from_scope(
                operations_result, included_child, op_key, new_scope_location
            )

    # Partially applies a scope argument to a given transformer.
    @staticmethod
    def _create_transformer_for_operation(
        given_transformer: Optional[Callable[[Scope, List[str]], List[str]]],
        transformer_scope: Scope,
    ) -> Callable[[List[str]], List[str]]:
        if given_transformer:

            def wrapped_transformer(values):
                return given_transformer(transformer_scope, values)

        else:

            def wrapped_transformer(values):
                return values

        return wrapped_transformer

    def _evalOps(
        self,
        key: str,
        transformer: Optional[Callable[[Scope, List[str]], List[str]]],
        result: List[str],
        *,
        inherit: bool = False,
    ) -> List[str]:
        self._visited_keys.add(key)

        # Inherit values from parent scope.
        # This is a strange edge case which is wrong in principle, because
        # .pro files are imperative and not declarative. Nevertheless
        # this fixes certain mappings (e.g. for handling
        # VERSIONTAGGING_SOURCES in src/corelib/global/global.pri).
        if self._parent and inherit:
            result = self._parent._evalOps(key, transformer, result)

        operations_to_run: List[Dict[str, Any]] = []
        starting_location = OperationLocation()
        starting_scope = self
        self._gather_operations_from_scope(
            operations_to_run, starting_scope, key, starting_location
        )

        # Sorts the operations based on the location of each operation. Technically compares two
        # lists of tuples.
        operations_to_run = sorted(operations_to_run, key=lambda o: o["location"])

        # Process the operations.
        for op_info in operations_to_run:
            op_transformer = self._create_transformer_for_operation(transformer, op_info["scope"])
            result = op_info["op"].process(key, result, op_transformer)
        return result

    def get(self, key: str, *, ignore_includes: bool = False, inherit: bool = False) -> List[str]:
        is_same_path = self.currentdir == self.basedir
        if not is_same_path:
            relative_path = posixpath.relpath(self.currentdir, self.basedir)

        if key == "QQC2_SOURCE_TREE":
            qmake_or_cmake_conf_path = find_qmake_or_cmake_conf(os.path.abspath(self.currentdir))
            qmake_or_cmake_conf_dir_path = os.path.dirname(qmake_or_cmake_conf_path)
            project_relative_path = os.path.relpath(qmake_or_cmake_conf_dir_path, self.currentdir)
            return ["${CMAKE_CURRENT_SOURCE_DIR}/" + project_relative_path]

        if key == "QT_ARCH":
            return ["${CMAKE_SYSTEM_PROCESSOR}"]

        if key == "_PRO_FILE_PWD_":
            return ["${CMAKE_CURRENT_SOURCE_DIR}"]
        if key == "PWD":
            if is_same_path:
                return ["${CMAKE_CURRENT_SOURCE_DIR}"]
            else:
                return [f"${{CMAKE_CURRENT_SOURCE_DIR}}/{relative_path}"]
        if key == "OUT_PWD":
            if is_same_path:
                return ["${CMAKE_CURRENT_BINARY_DIR}"]
            else:
                return [f"${{CMAKE_CURRENT_BINARY_DIR}}/{relative_path}"]

        # Horrible hack. If we're returning the values for some key
        # that looks like source or header files, make sure to use a
        # map_files transformer, so that $$PWD values are evaluated
        # in the transformer scope, otherwise relative paths will be
        # broken.
        # Looking at you qmltyperegistrar.pro.
        eval_ops_transformer = None
        if key.endswith("SOURCES") or key.endswith("HEADERS"):

            def file_transformer(scope, files):
                return scope._map_files(files)

            eval_ops_transformer = file_transformer
        return self._evalOps(key, eval_ops_transformer, [], inherit=inherit)

    def get_string(self, key: str, default: str = "", inherit: bool = False) -> str:
        v = self.get(key, inherit=inherit)
        if len(v) == 0:
            return default
        if len(v) > 1:
            return " ".join(v)
        return v[0]

    def _map_files(
        self, files: List[str], *, use_vpath: bool = True, is_include: bool = False
    ) -> List[str]:

        expanded_files = []  # type: List[str]
        for f in files:
            r = self._expand_value(f)
            expanded_files += r

        mapped_files = list(
            map(lambda f: map_to_file(f, self, is_include=is_include), expanded_files)
        )

        if use_vpath:
            result = list(
                map(
                    lambda f: handle_vpath(f, self.basedir, self.get("VPATH", inherit=True)),
                    mapped_files,
                )
            )
        else:
            result = mapped_files

        # strip ${CMAKE_CURRENT_SOURCE_DIR}:
        result = list(
            map(lambda f: f[28:] if f.startswith("${CMAKE_CURRENT_SOURCE_DIR}/") else f, result)
        )

        # strip leading ./:
        result = list(map(lambda f: trim_leading_dot(f), result))

        return result

    def get_files(
        self, key: str, *, use_vpath: bool = False, is_include: bool = False
    ) -> List[str]:
        def transformer(scope, files):
            return scope._map_files(files, use_vpath=use_vpath, is_include=is_include)

        return list(self._evalOps(key, transformer, []))

    @staticmethod
    def _replace_env_var_value(value: Any) -> Any:
        if not isinstance(value, str):
            return value

        pattern = re.compile(r"\$\$\(([A-Za-z_][A-Za-z0-9_]*)\)")
        match = re.search(pattern, value)
        if match:
            value = re.sub(pattern, r"$ENV{\1}", value)

        return value

    def _expand_value(self, value: str) -> List[str]:
        result = value
        pattern = re.compile(r"\$\$\{?([A-Za-z_][A-Za-z0-9_]*)\}?")
        match = re.search(pattern, result)
        while match:
            old_result = result
            match_group_0 = match.group(0)
            if match_group_0 == value:
                get_result = self.get(match.group(1), inherit=True)
                if len(get_result) == 1:
                    result = get_result[0]
                    result = self._replace_env_var_value(result)
                else:
                    # Recursively expand each value from the result list
                    # returned from self.get().
                    result_list: List[str] = []
                    for entry_value in get_result:
                        result_list += self._expand_value(self._replace_env_var_value(entry_value))
                    return result_list
            else:
                replacement = self.get(match.group(1), inherit=True)
                replacement_str = replacement[0] if replacement else ""
                if replacement_str == value:
                    # we have recursed
                    replacement_str = ""
                result = result[: match.start()] + replacement_str + result[match.end() :]
                result = self._replace_env_var_value(result)

            if result == old_result:
                return [result]  # Do not go into infinite loop

            match = re.search(pattern, result)

        result = self._replace_env_var_value(result)
        return [result]

    def expand(self, key: str) -> List[str]:
        value = self.get(key)
        result: List[str] = []
        assert isinstance(value, list)
        for v in value:
            result += self._expand_value(v)
        return result

    def expandString(self, key: str) -> str:
        result = self._expand_value(self.get_string(key))
        assert len(result) == 1
        return result[0]

    def _get_operation_at_index(self, key, index):
        return self._operations[key][index]

    @property
    def TEMPLATE(self) -> str:
        return self.get_string("TEMPLATE", "app")

    def _rawTemplate(self) -> str:
        return self.get_string("TEMPLATE")

    @property
    def TARGET(self) -> str:
        target = self.expandString("TARGET") or os.path.splitext(os.path.basename(self.file))[0]
        return re.sub(r"\.\./", "", target)

    @property
    def TARGET_ORIGINAL(self) -> str:
        return self.expandString("TARGET") or os.path.splitext(os.path.basename(self.file))[0]

    @property
    def _INCLUDED(self) -> List[str]:
        return self.get("_INCLUDED")


# Given "if(a|b):c" returns "(a|b):c". Uses pyparsing to keep the parentheses
# balanced.
def unwrap_if(input_string):
    # Compute the grammar only once.
    if not hasattr(unwrap_if, "if_grammar"):

        def handle_expr_with_parentheses(s, l_unused, t):
            # The following expression unwraps the condition via the
            # additional info set by originalTextFor, thus returning the
            # condition without parentheses.
            condition_without_parentheses = s[t._original_start + 1 : t._original_end - 1]

            # Re-add the parentheses, but with spaces in-between. This
            # fixes map_condition -> map_platform to apply properly.
            condition_with_parentheses = "( " + condition_without_parentheses + " )"
            return condition_with_parentheses

        expr_with_parentheses = pp.originalTextFor(pp.nestedExpr())
        expr_with_parentheses.setParseAction(handle_expr_with_parentheses)

        if_keyword = pp.Suppress(pp.Keyword("if"))
        unwrap_if.if_grammar = if_keyword + expr_with_parentheses

    output_string = unwrap_if.if_grammar.transformString(input_string)
    return output_string


def map_condition(condition: str) -> str:
    # Some hardcoded cases that are too bothersome to generalize.
    condition = re.sub(
        r"qtConfig\(opengles\.\)",
        r"(QT_FEATURE_opengles2 OR QT_FEATURE_opengles3 OR QT_FEATURE_opengles31 OR QT_FEATURE_opengles32)",
        condition,
    )
    condition = re.sub(
        r"qtConfig\(opengl\(es1\|es2\)\?\)",
        r"(QT_FEATURE_opengl OR QT_FEATURE_opengles2 OR QT_FEATURE_opengles3)",
        condition,
    )
    condition = re.sub(r"qtConfig\(opengl\.\*\)", r"QT_FEATURE_opengl", condition)
    condition = re.sub(r"^win\*$", r"win", condition)
    condition = re.sub(r"^no-png$", r"NOT QT_FEATURE_png", condition)
    condition = re.sub(r"contains\(CONFIG, static\)", r"NOT QT_BUILD_SHARED_LIBS", condition)
    condition = re.sub(r"contains\(QT_CONFIG,\w*shared\)", r"QT_BUILD_SHARED_LIBS", condition)
    condition = re.sub(r"CONFIG\(osx\)", r"MACOS", condition)

    def gcc_version_handler(match_obj: Match):
        operator = match_obj.group(1)
        version_type = match_obj.group(2)
        if operator == "equals":
            operator = "STREQUAL"
        elif operator == "greaterThan":
            operator = "STRGREATER"
        elif operator == "lessThan":
            operator = "STRLESS"

        version = match_obj.group(3)
        return f"(QT_COMPILER_VERSION_{version_type} {operator} {version})"

    # TODO: Possibly fix for other compilers.
    pattern = r"(equals|greaterThan|lessThan)\(QT_GCC_([A-Z]+)_VERSION,[ ]*([0-9]+)\)"
    condition = re.sub(pattern, gcc_version_handler, condition)

    def windows_sdk_version_handler(match_obj: Match):
        operator = match_obj.group(1)
        if operator == "equals":
            operator = "STREQUAL"
        elif operator == "greaterThan":
            operator = "STRGREATER"
        elif operator == "lessThan":
            operator = "STRLESS"

        version = match_obj.group(2)
        return f"(QT_WINDOWS_SDK_VERSION {operator} {version})"

    pattern = r"(equals|greaterThan|lessThan)\(WINDOWS_SDK_VERSION,[ ]*([0-9]+)\)"
    condition = re.sub(pattern, windows_sdk_version_handler, condition)

    def qt_version_handler(match_obj: Match):
        operator = match_obj.group(1)
        if operator == "equals":
            operator = "EQUAL"
        elif operator == "greaterThan":
            operator = "GREATER"
        elif operator == "lessThan":
            operator = "LESS"

        operator_prefix = "VERSION_"
        version_variable = "QT_VERSION"
        version_flavor = match_obj.group(2)
        if version_flavor:
            version_variable += "_" + version_flavor[:-1]
            operator_prefix = ""

        version = match_obj.group(3)
        return f"({version_variable} {operator_prefix}{operator} {version})"

    pattern = r"(equals|greaterThan|lessThan)\(QT_(MAJOR_|MINOR_|PATCH_)?VERSION,[ ]*([0-9.]+)\)"
    condition = re.sub(pattern, qt_version_handler, condition)

    # Generic lessThan|equals|lessThan()

    def generic_version_handler(match_obj: Match):
        operator = match_obj.group(1)
        if operator == "equals":
            operator = "EQUAL"
        elif operator == "greaterThan":
            operator = "GREATER"
        elif operator == "lessThan":
            operator = "LESS"

        variable = match_obj.group(2)
        version = match_obj.group(3)
        return f"({variable} {operator} {version})"

    pattern = r"(equals|greaterThan|lessThan)\(([^,]+?),[ ]*([0-9]+)\)"
    condition = re.sub(pattern, generic_version_handler, condition)

    # Handle if(...) conditions.
    condition = unwrap_if(condition)

    condition = re.sub(r"\bisEmpty\s*\((.*?)\)", r"\1_ISEMPTY", condition)
    condition = re.sub(
        r"\bcontains\s*\(\s*(?:QT_)?CONFIG\s*,\s*c\+\+(\d+)\)",
        r"cxx_std_\1 IN_LIST CMAKE_CXX_COMPILE_FEATURES",
        condition,
    )
    condition = re.sub(r'\bcontains\s*\((.*?),\s*"?(.*?)"?\)', r"\1___contains___\2", condition)
    condition = re.sub(r'\bequals\s*\((.*?),\s*"?(.*?)"?\)', r"\1___equals___\2", condition)
    condition = re.sub(r'\bisEqual\s*\((.*?),\s*"?(.*?)"?\)', r"\1___equals___\2", condition)
    condition = re.sub(r"\s*==\s*", "___STREQUAL___", condition)
    condition = re.sub(r"\bexists\s*\((.*?)\)", r"EXISTS \1", condition)

    # checking mkspec, predating gcc scope in qmake, will then be replaced by platform_mapping in helper.py
    condition = condition.replace("*-g++*", "GCC")
    condition = condition.replace("*g++*", "GCC")
    condition = condition.replace("aix-g++*", "AIX")
    condition = condition.replace("*-icc*", "ICC")
    condition = condition.replace("*-clang*", "CLANG")
    condition = condition.replace("*-llvm", "CLANG")
    condition = condition.replace("win32-*", "WIN32")

    pattern = r"CONFIG\((debug|release),debug\|release\)"
    match_result = re.match(pattern, condition)
    if match_result:
        build_type = match_result.group(1)
        if build_type == "debug":
            build_type = "Debug"
        elif build_type == "release":
            build_type = "Release"
        condition = re.sub(pattern, f"(CMAKE_BUILD_TYPE STREQUAL {build_type})", condition)

    condition = condition.replace("*", "_x_")
    condition = condition.replace(".$$", "__ss_")
    condition = condition.replace("$$", "_ss_")

    condition = condition.replace("!", "NOT ")
    condition = condition.replace("&&", " AND ")
    condition = condition.replace("|", " OR ")

    # new conditions added by the android multi arch qmake build
    condition = re.sub(r"(^| )x86((?=[^\w])|$)", "TEST_architecture_arch STREQUAL i386", condition)
    condition = re.sub(r"(^| )x86_64", " TEST_architecture_arch STREQUAL x86_64", condition)
    condition = re.sub(r"(^| )arm64-v8a", "TEST_architecture_arch STREQUAL arm64", condition)
    condition = re.sub(r"(^| )armeabi-v7a", "TEST_architecture_arch STREQUAL arm", condition)

    # some defines replacements
    condition = re.sub(r"DEFINES___contains___QT_NO_CURSOR", r"(NOT QT_FEATURE_cursor)", condition)
    condition = re.sub(
        r"DEFINES___contains___QT_NO_TRANSLATION", r"(NOT QT_FEATURE_translation)", condition
    )
    condition = re.sub(r"styles___contains___fusion", r"QT_FEATURE_style_fusion", condition)
    condition = re.sub(r"CONFIG___contains___largefile", r"QT_FEATURE_largefile", condition)

    condition = condition.replace("cross_compile", "CMAKE_CROSSCOMPILING")

    cmake_condition = ""
    for part in condition.split():
        # some features contain e.g. linux, that should not be
        # turned upper case
        feature = re.match(r"(qtConfig|qtHaveModule)\(([a-zA-Z0-9_-]+)\)", part)
        if feature:
            if feature.group(1) == "qtHaveModule":
                part = f"TARGET {map_qt_library(feature.group(2))}"
            else:
                feature_name = featureName(feature.group(2))
                if (
                    feature_name.startswith("system_")
                    and is_known_3rd_party_library(feature_name[7:])
                    and not feature_name.startswith("system_jpeg")
                    and not feature_name.startswith("system_zlib")
                    and not feature_name.startswith("system_tiff")
                    and not feature_name.startswith("system_assimp")
                    and not feature_name.startswith("system_doubleconversion")
                    and not feature_name.startswith("system_sqlite")
                    and not feature_name.startswith("system_hunspell")
                    and not feature_name.startswith("system_libb2")
                    and not feature_name.startswith("system_webp")
                ):
                    part = "ON"
                elif feature == "dlopen":
                    part = "ON"
                else:
                    part = "QT_FEATURE_" + feature_name
        else:
            part = map_platform(part)

        part = part.replace("true", "ON")
        part = part.replace("false", "OFF")
        cmake_condition += " " + part
    return cmake_condition.strip()


_path_replacements = {
    "$$[QT_INSTALL_PREFIX]": "${INSTALL_DIRECTORY}",
    "$$[QT_INSTALL_EXAMPLES]": "${INSTALL_EXAMPLESDIR}",
    "$$[QT_INSTALL_TESTS]": "${INSTALL_TESTSDIR}",
    "$$OUT_PWD": "${CMAKE_CURRENT_BINARY_DIR}",
    "$$MODULE_BASE_OUTDIR": "${QT_BUILD_DIR}",
}


def replace_path_constants(path: str, scope: Scope) -> str:
    """Clean up DESTDIR and target.path"""
    if path.startswith("./"):
        path = f"${{CMAKE_CURRENT_BINARY_DIR}}/{path[2:]}"
    elif path.startswith("../"):
        path = f"${{CMAKE_CURRENT_BINARY_DIR}}/{path}"
    for original, replacement in _path_replacements.items():
        path = path.replace(original, replacement)
    path = path.replace("$$TARGET", scope.TARGET)
    return path


def handle_subdir(
    scope: Scope,
    cm_fh: IO[str],
    *,
    indent: int = 0,
    is_example: bool = False,
    is_user_project: bool = False,
) -> None:

    # Global nested dictionary that will contain sub_dir assignments and their conditions.
    # Declared as a global in order not to pollute the nested function signatures with giant
    # type hints.
    sub_dirs: Dict[str, Dict[str, Set[FrozenSet[str]]]] = {}

    # Collects assignment conditions into global sub_dirs dict.
    def collect_subdir_info(sub_dir_assignment: str, *, current_conditions: FrozenSet[str] = None):
        subtraction = sub_dir_assignment.startswith("-")
        if subtraction:
            subdir_name = sub_dir_assignment[1:]
        else:
            subdir_name = sub_dir_assignment
        if subdir_name not in sub_dirs:
            sub_dirs[subdir_name] = {}
        additions = sub_dirs[subdir_name].get("additions", set())
        subtractions = sub_dirs[subdir_name].get("subtractions", set())
        if current_conditions:
            if subtraction:
                subtractions.add(current_conditions)
            else:
                additions.add(current_conditions)
        if additions:
            sub_dirs[subdir_name]["additions"] = additions
        if subtractions:
            sub_dirs[subdir_name]["subtractions"] = subtractions

    # Recursive helper that collects subdir info for given scope,
    # and the children of the given scope.
    def handle_subdir_helper(
        scope: Scope,
        cm_fh: IO[str],
        *,
        indent: int = 0,
        current_conditions: FrozenSet[str] = frozenset(),
        is_example: bool = False,
    ):
        for sd in scope.get_files("SUBDIRS"):
            # Collect info about conditions and SUBDIR assignments in the
            # current scope.
            if os.path.isdir(sd) or sd.startswith("-"):
                collect_subdir_info(sd, current_conditions=current_conditions)
            # For the file case, directly write into the file handle.
            elif os.path.isfile(sd):
                # Handle cases with SUBDIRS += Foo/bar/z.pro. We want to be able
                # to generate add_subdirectory(Foo/bar) instead of parsing the full
                # .pro file in the current CMakeLists.txt. This causes issues
                # with relative paths in certain projects otherwise.
                dirname = os.path.dirname(sd)
                if dirname:
                    collect_subdir_info(dirname, current_conditions=current_conditions)
                else:
                    subdir_result, project_file_content = parseProFile(sd, debug=False)
                    subdir_scope = Scope.FromDict(
                        scope,
                        sd,
                        subdir_result.asDict().get("statements"),
                        "",
                        scope.basedir,
                        project_file_content=project_file_content,
                    )

                    do_include(subdir_scope)
                    cmakeify_scope(
                        subdir_scope,
                        cm_fh,
                        indent=indent,
                        is_example=is_example,
                        is_user_project=is_user_project,
                    )
            else:
                print(f"    XXXX: SUBDIR {sd} in {scope}: Not found.")

        # Collect info about conditions and SUBDIR assignments in child
        # scopes, aka recursively call the same function, but with an
        # updated current_conditions frozen set.
        for c in scope.children:
            # Use total_condition for 'else' conditions, otherwise just use the regular value to
            # simplify the logic.
            child_conditions = current_conditions
            child_condition = c.total_condition if c.condition == "else" else c.condition
            if child_condition:
                child_conditions = frozenset((*child_conditions, child_condition))

            handle_subdir_helper(
                c,
                cm_fh,
                indent=indent + 1,
                current_conditions=child_conditions,
                is_example=is_example,
            )

    def group_and_print_sub_dirs(scope: Scope, indent: int = 0) -> None:
        # Simplify conditions, and group
        # subdirectories with the same conditions.
        grouped_sub_dirs: Dict[str, List[str]] = {}

        # Wraps each element in the given interable with parentheses,
        # to make sure boolean simplification happens correctly.
        def wrap_in_parenthesis(iterable):
            return [f"({c})" for c in iterable]

        def join_all_conditions(set_of_alternatives):
            # Elements within one frozen set represent one single
            # alternative whose pieces are ANDed together.
            # This is repeated for each alternative that would
            # enable a subdir, and are thus ORed together.
            final_str = ""
            if set_of_alternatives:
                wrapped_set_of_alternatives = [
                    wrap_in_parenthesis(alternative) for alternative in set_of_alternatives
                ]
                alternatives = [
                    f'({" AND ".join(alternative)})' for alternative in wrapped_set_of_alternatives
                ]
                final_str = " OR ".join(sorted(alternatives))
            return final_str

        for subdir_name in sub_dirs:
            additions = sub_dirs[subdir_name].get("additions", set())
            subtractions = sub_dirs[subdir_name].get("subtractions", set())

            # An empty condition key represents the group of sub dirs
            # that should be added unconditionally.
            condition_key = ""
            if additions or subtractions:
                addition_str = join_all_conditions(additions)
                if addition_str:
                    addition_str = f"({addition_str})"
                subtraction_str = join_all_conditions(subtractions)
                if subtraction_str:
                    subtraction_str = f"NOT ({subtraction_str})"

                condition_str = addition_str
                if condition_str and subtraction_str:
                    condition_str += " AND "
                condition_str += subtraction_str
                if not condition_str.rstrip("()").strip():
                    continue
                condition_simplified = simplify_condition(condition_str)
                condition_key = condition_simplified

            sub_dir_list_by_key: List[str] = grouped_sub_dirs.get(condition_key, [])
            sub_dir_list_by_key.append(subdir_name)
            grouped_sub_dirs[condition_key] = sub_dir_list_by_key

        # Print any requires() blocks.
        cm_fh.write(expand_project_requirements(scope, skip_message=True))

        # Print the groups.
        ind = spaces(indent)
        for condition_key in grouped_sub_dirs:
            cond_ind = ind
            if condition_key:
                cm_fh.write(f"{ind}if({condition_key})\n")
                cond_ind += "    "

            sub_dir_list_by_key = grouped_sub_dirs.get(condition_key, [])
            for subdir_name in sub_dir_list_by_key:
                cm_fh.write(f"{cond_ind}add_subdirectory({subdir_name})\n")
            if condition_key:
                cm_fh.write(f"{ind}endif()\n")

    # A set of conditions which will be ANDed together. The set is recreated with more conditions
    # as the scope deepens.
    current_conditions: FrozenSet[str] = frozenset()

    # Compute the total condition for scopes. Needed for scopes that
    # have 'else' as a condition.
    recursive_evaluate_scope(scope)

    # Do the work.
    handle_subdir_helper(
        scope, cm_fh, indent=indent, current_conditions=current_conditions, is_example=is_example
    )

    # Make sure to exclude targets within subdirectories first.
    qt_no_make_tools = scope.get("_QT_NO_MAKE_TOOLS")
    if qt_no_make_tools:
        ind = spaces(indent + 1)
        directories_string = ""
        for directory in qt_no_make_tools:
            directories_string += f"{ind}{directory}\n"
        cm_fh.write(
            f"\nqt_exclude_tool_directories_from_default_target(\n{directories_string})\n\n"
        )

    # Then write the subdirectories.
    group_and_print_sub_dirs(scope, indent=indent)


def sort_sources(sources: List[str]) -> List[str]:
    to_sort = {}  # type: Dict[str, List[str]]
    for s in sources:
        if s is None:
            continue

        path = os.path.dirname(s)
        base = os.path.splitext(os.path.basename(s))[0]
        if base.endswith("_p"):
            base = base[:-2]
        sort_name = posixpath.join(path, base)

        array = to_sort.get(sort_name, [])
        array.append(s)

        to_sort[sort_name] = array

    lines = []
    for k in sorted(to_sort.keys()):
        lines.append(" ".join(sorted(to_sort[k])))

    return lines


def _map_libraries_to_cmake(
    libraries: List[str], known_libraries: Set[str], is_example: bool = False
) -> List[str]:
    result = []  # type: List[str]
    is_framework = False

    for lib in libraries:
        if lib == "-framework":
            is_framework = True
            continue
        if is_framework:
            if is_example:
                lib = f'"-framework {lib}"'
            else:
                lib = f"${{FW{lib}}}"
        if lib.startswith("-l"):
            lib = lib[2:]

        if lib.startswith("-"):
            lib = f"# Remove: {lib[1:]}"
        else:
            lib = map_3rd_party_library(lib)

        if not lib or lib in result or lib in known_libraries:
            continue

        result.append(lib)
        is_framework = False

    return result


def extract_cmake_libraries(
    scope: Scope, *, known_libraries: Optional[Set[str]] = None, is_example: bool = False
) -> Tuple[List[str], List[str]]:
    if known_libraries is None:
        known_libraries = set()
    public_dependencies = []  # type: List[str]
    private_dependencies = []  # type: List[str]

    for key in ["QMAKE_USE", "LIBS"]:
        public_dependencies += scope.expand(key)
    for key in ["QMAKE_USE_PRIVATE", "QMAKE_USE_FOR_PRIVATE", "LIBS_PRIVATE"]:
        private_dependencies += scope.expand(key)

    for key in ["QT_FOR_PRIVATE", "QT_PRIVATE"]:
        private_dependencies += [map_qt_library(q) for q in scope.expand(key)]

    for key in ["QT"]:
        for lib in scope.expand(key):
            mapped_lib = map_qt_library(lib)
            public_dependencies.append(mapped_lib)

    return (
        _map_libraries_to_cmake(public_dependencies, known_libraries, is_example=is_example),
        _map_libraries_to_cmake(private_dependencies, known_libraries, is_example=is_example),
    )


def write_header(cm_fh: IO[str], name: str, typename: str, *, indent: int = 0):
    ind = spaces(indent)
    comment_line = "#" * 69
    cm_fh.write(f"{ind}{comment_line}\n")
    cm_fh.write(f"{ind}## {name} {typename}:\n")
    cm_fh.write(f"{ind}{comment_line}\n\n")


def write_scope_header(cm_fh: IO[str], *, indent: int = 0):
    ind = spaces(indent)
    comment_line = "#" * 69
    cm_fh.write(f"\n{ind}## Scopes:\n")
    cm_fh.write(f"{ind}{comment_line}\n")


def write_list(
    cm_fh: IO[str],
    entries: List[str],
    cmake_parameter: str,
    indent: int = 0,
    *,
    header: str = "",
    footer: str = "",
    prefix: str = "",
):
    if not entries:
        return

    ind = spaces(indent)
    extra_indent = ""

    if header:
        cm_fh.write(f"{ind}{header}")
        extra_indent += "    "
    if cmake_parameter:
        cm_fh.write(f"{ind}{extra_indent}{cmake_parameter}\n")
        extra_indent += "    "
    for s in sort_sources(entries):
        cm_fh.write(f"{ind}{extra_indent}{prefix}{s}\n")
    if footer:
        cm_fh.write(f"{ind}{footer}\n")


def write_source_file_list(
    cm_fh: IO[str],
    scope,
    cmake_parameter: str,
    keys: List[str],
    indent: int = 0,
    *,
    header: str = "",
    footer: str = "",
):
    # collect sources
    sources: List[str] = []
    for key in keys:
        sources += scope.get_files(key, use_vpath=True)

    # Remove duplicates, like in the case when NO_PCH_SOURCES ends up
    # adding the file to SOURCES, but SOURCES might have already
    # contained it before. Preserves order in Python 3.7+ because
    # dict keys are ordered.
    sources = list(dict.fromkeys(sources))

    write_list(cm_fh, sources, cmake_parameter, indent, header=header, footer=footer)


def write_all_source_file_lists(
    cm_fh: IO[str],
    scope: Scope,
    header: str,
    *,
    indent: int = 0,
    footer: str = "",
    extra_keys: Optional[List[str]] = None,
):
    if extra_keys is None:
        extra_keys = []
    write_source_file_list(
        cm_fh,
        scope,
        header,
        ["SOURCES", "HEADERS", "OBJECTIVE_SOURCES", "OBJECTIVE_HEADERS", "NO_PCH_SOURCES", "FORMS"]
        + extra_keys,
        indent,
        footer=footer,
    )


def write_defines(
    cm_fh: IO[str], scope: Scope, cmake_parameter: str, *, indent: int = 0, footer: str = ""
):
    defines = scope.expand("DEFINES")
    defines += [d[2:] for d in scope.expand("QMAKE_CXXFLAGS") if d.startswith("-D")]
    defines = [
        d.replace('=\\\\\\"$$PWD/\\\\\\"', '="${CMAKE_CURRENT_SOURCE_DIR}/"') for d in defines
    ]

    # Handle LIBS_SUFFIX='\\"_$${QT_ARCH}.so\\"'.
    # The escaping of backslashes is still needed even if it's a raw
    # string, because backslashes have a special meaning for regular
    # expressions (escape next char). So we actually expect to match
    # 2 backslashes in the input string.
    pattern = r"""([^ ]+)='\\\\"([^ ]*)\\\\"'"""

    # Replace with regular quotes, CMake will escape the quotes when
    # passing the define to the compiler.
    replacement = r'\1="\2"'
    defines = [re.sub(pattern, replacement, d) for d in defines]

    if "qml_debug" in scope.get("CONFIG"):
        defines.append("QT_QML_DEBUG")

    write_list(cm_fh, defines, cmake_parameter, indent, footer=footer)


def write_3rd_party_defines(
    cm_fh: IO[str], scope: Scope, cmake_parameter: str, *, indent: int = 0, footer: str = ""
):
    defines = scope.expand("MODULE_DEFINES")
    write_list(cm_fh, defines, cmake_parameter, indent, footer=footer)


def get_include_paths_helper(scope: Scope, include_var_name: str) -> List[str]:
    includes = [i.rstrip("/") or ("/") for i in scope.get_files(include_var_name)]
    return includes


def write_include_paths(
    cm_fh: IO[str], scope: Scope, cmake_parameter: str, *, indent: int = 0, footer: str = ""
):
    includes = get_include_paths_helper(scope, "INCLUDEPATH")
    write_list(cm_fh, includes, cmake_parameter, indent, footer=footer)


def write_3rd_party_include_paths(
    cm_fh: IO[str], scope: Scope, cmake_parameter: str, *, indent: int = 0, footer: str = ""
):
    # Used in qt_helper_lib.prf.
    includes = get_include_paths_helper(scope, "MODULE_INCLUDEPATH")

    # Wrap the includes in BUILD_INTERFACE generator expression, because
    # the include paths point to a source dir, and CMake will error out
    # when trying to create consumable exported targets.
    processed_includes = []
    for i in includes:
        # CMake generator expressions don't seem to like relative paths.
        # Make them absolute relative to the source dir.
        if is_path_relative_ish(i):
            i = f"${{CMAKE_CURRENT_SOURCE_DIR}}/{i}"
        i = f"$<BUILD_INTERFACE:{i}>"
        processed_includes.append(i)

    write_list(cm_fh, processed_includes, cmake_parameter, indent, footer=footer)


def write_compile_options(
    cm_fh: IO[str], scope: Scope, cmake_parameter: str, *, indent: int = 0, footer: str = ""
):
    compile_options = [d for d in scope.expand("QMAKE_CXXFLAGS") if not d.startswith("-D")]

    write_list(cm_fh, compile_options, cmake_parameter, indent, footer=footer)


# Return True if given scope belongs to a public module.
# First, traverse the parent/child hierarchy. Then, traverse the include hierarchy.
def recursive_is_public_module(scope: Scope):
    if scope.is_public_module:
        return True
    if scope.parent:
        return recursive_is_public_module(scope.parent)
    if scope.including_scope:
        return recursive_is_public_module(scope.including_scope)
    return False


def write_library_section(
    cm_fh: IO[str], scope: Scope, *, indent: int = 0, known_libraries: Optional[Set[str]] = None
):
    if known_libraries is None:
        known_libraries = set()
    public_dependencies, private_dependencies = extract_cmake_libraries(
        scope, known_libraries=known_libraries
    )

    is_public_module = recursive_is_public_module(scope)

    # When handling module dependencies, handle QT += foo-private magic.
    # This implies:
    #  target_link_libraries(Module PUBLIC Qt::Foo)
    #  target_link_libraries(Module PRIVATE Qt::FooPrivate)
    #  target_link_libraries(ModulePrivate INTERFACE Qt::FooPrivate)
    if is_public_module:
        private_module_dep_pattern = re.compile(r"^(Qt::(.+))Private$")

        public_module_public_deps = []
        public_module_private_deps = private_dependencies
        private_module_interface_deps = []

        for dep in public_dependencies:
            match = re.match(private_module_dep_pattern, dep)
            if match:
                if match[1] not in public_module_public_deps:
                    public_module_public_deps.append(match[1])
                private_module_interface_deps.append(dep)
                if dep not in public_module_private_deps:
                    public_module_private_deps.append(dep)
            else:
                if dep not in public_module_public_deps:
                    public_module_public_deps.append(dep)

        private_module_interface_deps.extend(
            [map_qt_library(q) for q in scope.expand("QT_FOR_PRIVATE")]
        )
        private_module_interface_deps.extend(
            _map_libraries_to_cmake(scope.expand("QMAKE_USE_FOR_PRIVATE"), known_libraries)
        )

        write_list(cm_fh, public_module_private_deps, "LIBRARIES", indent + 1)
        write_list(cm_fh, public_module_public_deps, "PUBLIC_LIBRARIES", indent + 1)
        write_list(cm_fh, private_module_interface_deps, "PRIVATE_MODULE_INTERFACE", indent + 1)
    else:
        write_list(cm_fh, private_dependencies, "LIBRARIES", indent + 1)
        write_list(cm_fh, public_dependencies, "PUBLIC_LIBRARIES", indent + 1)


def write_autogen_section(cm_fh: IO[str], scope: Scope, *, indent: int = 0):
    forms = scope.get_files("FORMS")
    if forms:
        write_list(cm_fh, ["uic"], "ENABLE_AUTOGEN_TOOLS", indent)


def write_sources_section(
    cm_fh: IO[str], scope: Scope, *, indent: int = 0, known_libraries: Optional[Set[str]] = None
):
    if known_libraries is None:
        known_libraries = set()
    ind = spaces(indent)

    # mark RESOURCES as visited:
    scope.get("RESOURCES")

    write_all_source_file_lists(cm_fh, scope, "SOURCES", indent=indent + 1)

    write_source_file_list(cm_fh, scope, "DBUS_ADAPTOR_SOURCES", ["DBUS_ADAPTORS"], indent + 1)
    dbus_adaptor_flags = scope.expand("QDBUSXML2CPP_ADAPTOR_HEADER_FLAGS")
    if dbus_adaptor_flags:
        dbus_adaptor_flags_line = '" "'.join(dbus_adaptor_flags)
        cm_fh.write(f"{ind}    DBUS_ADAPTOR_FLAGS\n")
        cm_fh.write(f'{ind}        "{dbus_adaptor_flags_line}"\n')

    write_source_file_list(cm_fh, scope, "DBUS_INTERFACE_SOURCES", ["DBUS_INTERFACES"], indent + 1)
    dbus_interface_flags = scope.expand("QDBUSXML2CPP_INTERFACE_HEADER_FLAGS")
    if dbus_interface_flags:
        dbus_interface_flags_line = '" "'.join(dbus_interface_flags)
        cm_fh.write(f"{ind}    DBUS_INTERFACE_FLAGS\n")
        cm_fh.write(f'{ind}        "{dbus_interface_flags_line}"\n')

    write_defines(cm_fh, scope, "DEFINES", indent=indent + 1)

    write_3rd_party_defines(cm_fh, scope, "PUBLIC_DEFINES", indent=indent + 1)

    write_include_paths(cm_fh, scope, "INCLUDE_DIRECTORIES", indent=indent + 1)

    write_3rd_party_include_paths(cm_fh, scope, "PUBLIC_INCLUDE_DIRECTORIES", indent=indent + 1)

    write_library_section(cm_fh, scope, indent=indent, known_libraries=known_libraries)

    write_compile_options(cm_fh, scope, "COMPILE_OPTIONS", indent=indent + 1)

    write_autogen_section(cm_fh, scope, indent=indent + 1)

    link_options = scope.get("QMAKE_LFLAGS")
    if link_options:
        cm_fh.write(f"{ind}    LINK_OPTIONS\n")
        for lo in link_options:
            cm_fh.write(f'{ind}        "{lo}"\n')

    moc_options = scope.get("QMAKE_MOC_OPTIONS")
    if moc_options:
        cm_fh.write(f"{ind}    MOC_OPTIONS\n")
        for mo in moc_options:
            cm_fh.write(f'{ind}        "{mo}"\n')

    precompiled_header = scope.get("PRECOMPILED_HEADER")
    if precompiled_header:
        cm_fh.write(f"{ind}    PRECOMPILED_HEADER\n")
        for header in precompiled_header:
            cm_fh.write(f'{ind}        "{header}"\n')

    no_pch_sources = scope.get("NO_PCH_SOURCES")
    if no_pch_sources:
        cm_fh.write(f"{ind}    NO_PCH_SOURCES\n")
        for source in no_pch_sources:
            cm_fh.write(f'{ind}        "{source}"\n')


def is_simple_condition(condition: str) -> bool:
    return " " not in condition or (condition.startswith("NOT ") and " " not in condition[4:])


def write_ignored_keys(scope: Scope, indent: str) -> str:
    result = ""
    ignored_keys = scope.keys - scope.visited_keys
    for k in sorted(ignored_keys):
        if k in {
            "_INCLUDED",
            "_LOADED",
            "TARGET",
            "QMAKE_DOCS",
            "QT_SOURCE_TREE",
            "QT_BUILD_TREE",
            "QTRO_SOURCE_TREE",
            "TRACEPOINT_PROVIDER",
            "PLUGIN_TYPE",
            "PLUGIN_CLASS_NAME",
            "CLASS_NAME",
            "MODULE_PLUGIN_TYPES",
        }:
            # All these keys are actually reported already
            continue
        values = scope.get(k)
        value_string = "<EMPTY>" if not values else '"' + '" "'.join(scope.get(k)) + '"'
        result += f"{indent}# {k} = {value_string}\n"

    if result:
        result = f"\n#### Keys ignored in scope {scope}:\n{result}"

    return result


def recursive_evaluate_scope(
    scope: Scope, parent_condition: str = "", previous_condition: str = ""
) -> str:
    current_condition = scope.condition
    total_condition = current_condition
    if total_condition == "else":
        assert previous_condition, f"Else branch without previous condition in: {scope.file}"
        total_condition = f"NOT ({previous_condition})"
    if parent_condition:
        if not total_condition:
            total_condition = parent_condition
        else:
            total_condition = f"({parent_condition}) AND ({total_condition})"

    scope.total_condition = simplify_condition(total_condition)

    prev_condition = ""
    for c in scope.children:
        prev_condition = recursive_evaluate_scope(c, total_condition, prev_condition)

    return current_condition


def map_to_cmake_condition(condition: str = "") -> str:
    condition = condition.replace("QTDIR_build", "QT_BUILDING_QT")
    condition = re.sub(
        r"\bQT_ARCH___equals___([a-zA-Z_0-9]*)",
        r'(TEST_architecture_arch STREQUAL "\1")',
        condition or "",
    )
    condition = re.sub(
        r"\bQT_ARCH___contains___([a-zA-Z_0-9]*)",
        r'(TEST_architecture_arch STREQUAL "\1")',
        condition or "",
    )
    condition = condition.replace("QT___contains___opengl", "QT_FEATURE_opengl")
    condition = condition.replace("QT___contains___widgets", "QT_FEATURE_widgets")
    condition = condition.replace(
        "DEFINES___contains___QT_NO_PRINTER", "(QT_FEATURE_printer EQUAL FALSE)"
    )
    return condition


resource_file_expansion_counter = 0


def expand_resource_glob(cm_fh: IO[str], expression: str) -> str:
    global resource_file_expansion_counter
    r = expression.replace('"', "")

    cm_fh.write(
        dedent(
            f"""
        file(GLOB resource_glob_{resource_file_expansion_counter} RELATIVE "${{CMAKE_CURRENT_SOURCE_DIR}}" "{r}")
        foreach(file IN LISTS resource_glob_{resource_file_expansion_counter})
            set_source_files_properties("${{CMAKE_CURRENT_SOURCE_DIR}}/${{file}}" PROPERTIES QT_RESOURCE_ALIAS "${{file}}")
        endforeach()
        """
        )
    )

    expanded_var = f"${{resource_glob_{resource_file_expansion_counter}}}"
    resource_file_expansion_counter += 1
    return expanded_var


def extract_resources(
    target: str,
    scope: Scope,
) -> Tuple[List[QtResource], List[str]]:
    """Read the resources of the given scope.
    Return a tuple:
      - list of QtResource objects
      - list of standalone sources files that are marked as QTQUICK_COMPILER_SKIPPED_RESOURCES"""

    resource_infos: List[QtResource] = []
    skipped_standalone_files: List[str] = []

    resources = scope.get_files("RESOURCES")
    qtquickcompiler_skipped = scope.get_files("QTQUICK_COMPILER_SKIPPED_RESOURCES")
    if resources:
        standalone_files: List[str] = []
        for r in resources:
            skip_qtquick_compiler = r in qtquickcompiler_skipped
            if r.endswith(".qrc"):
                if "${CMAKE_CURRENT_BINARY_DIR}" in r:
                    resource_infos.append(
                        QtResource(
                            name=r, generated=True, skip_qtquick_compiler=skip_qtquick_compiler
                        )
                    )
                    continue
                resource_infos += read_qrc_file(
                    r,
                    scope.basedir,
                    scope.file_absolute_path,
                    skip_qtquick_compiler=skip_qtquick_compiler,
                )
            else:
                immediate_files = {f: "" for f in scope.get_files(f"{r}.files")}
                if immediate_files:
                    immediate_files_filtered = []
                    for f in immediate_files:
                        immediate_files_filtered.append(f)
                    immediate_files = {f: "" for f in immediate_files_filtered}
                    scope_prefix = scope.get(f"{r}.prefix")
                    if scope_prefix:
                        immediate_prefix = scope_prefix[0]
                    else:
                        immediate_prefix = "/"
                    immediate_base_list = scope.get(f"{r}.base")
                    assert (
                        len(immediate_base_list) < 2
                    ), "immediate base directory must be at most one entry"
                    immediate_base = replace_path_constants("".join(immediate_base_list), scope)
                    immediate_lang = None
                    immediate_name = f"qmake_{r}"
                    resource_infos.append(
                        QtResource(
                            name=immediate_name,
                            prefix=immediate_prefix,
                            base_dir=immediate_base,
                            lang=immediate_lang,
                            files=immediate_files,
                            skip_qtquick_compiler=skip_qtquick_compiler,
                        )
                    )
                else:
                    standalone_files.append(r)
                    if not ("*" in r) and skip_qtquick_compiler:
                        skipped_standalone_files.append(r)

        if standalone_files:
            resource_infos.append(
                QtResource(
                    name="qmake_immediate",
                    prefix="/",
                    base_dir="",
                    files={f: "" for f in standalone_files},
                )
            )

    return (resource_infos, skipped_standalone_files)


def write_resources(
    cm_fh: IO[str],
    target: str,
    scope: Scope,
    indent: int = 0,
    is_example=False,
    target_ref: str = None,
    resources: List[QtResource] = None,
    skipped_standalone_files: List[str] = None,
):
    if resources is None:
        (resources, skipped_standalone_files) = extract_resources(target, scope)
    if target_ref is None:
        target_ref = target

    qrc_output = ""
    for r in resources:
        name = r.name
        if "*" in name:
            name = expand_resource_glob(cm_fh, name)
        qrc_output += write_add_qt_resource_call(
            target=target_ref,
            scope=scope,
            resource_name=name,
            prefix=r.prefix,
            base_dir=r.base_dir,
            lang=r.lang,
            files=r.files,
            skip_qtquick_compiler=r.skip_qtquick_compiler,
            is_example=is_example,
        )

    if skipped_standalone_files:
        for f in skipped_standalone_files:
            qrc_output += (
                f'set_source_files_properties("{f}" PROPERTIES ' f"QT_SKIP_QUICKCOMPILER 1)\n\n"
            )

    if qrc_output:
        str_indent = spaces(indent)
        cm_fh.write(f"\n{str_indent}# Resources:\n")
        for line in qrc_output.split("\n"):
            if line:
                cm_fh.write(f"{str_indent}{line}\n")
            else:
                # do not add spaces to empty lines
                cm_fh.write("\n")


def write_statecharts(cm_fh: IO[str], target: str, scope: Scope, indent: int = 0, is_example=False):
    sources = scope.get_files("STATECHARTS", use_vpath=True)
    if not sources:
        return
    cm_fh.write("\n# Statecharts:\n")
    if is_example:
        cm_fh.write(f"qt6_add_statecharts({target}\n")
    else:
        cm_fh.write(f"add_qt_statecharts({target} FILES\n")
    indent += 1
    for f in sources:
        cm_fh.write(f"{spaces(indent)}{f}\n")
    cm_fh.write(")\n")


def write_qlalrsources(cm_fh: IO[str], target: str, scope: Scope, indent: int = 0):
    sources = scope.get_files("QLALRSOURCES", use_vpath=True)
    if not sources:
        return
    cm_fh.write("\n# QLALR Grammars:\n")
    cm_fh.write("qt_process_qlalr(\n")
    indent += 1
    cm_fh.write(f"{spaces(indent)}{target}\n")
    cm_fh.write(f"{spaces(indent)}{';'.join(sources)}\n")
    cm_fh.write(f'{spaces(indent)}""\n')
    cm_fh.write(")\n")


def write_repc_files(cm_fh: IO[str], target: str, scope: Scope, indent: int = 0):
    for t in ["SOURCE", "REPLICA", "MERGED"]:
        sources = scope.get_files("REPC_" + t, use_vpath=True)
        if not sources:
            continue
        cm_fh.write(f"qt6_add_repc_{t.lower()}({target}\n")
        indent += 1
        for f in sources:
            cm_fh.write(f"{spaces(indent)}{f}\n")
        cm_fh.write(")\n")


def write_generic_cmake_command(
    cm_fh: IO[str], command_name: str, arguments: List[str], indent: int = 0
):
    ind = spaces(indent)
    arguments_str = " ".join(arguments)
    cm_fh.write(f"{ind}{command_name}({arguments_str})\n")


def write_set_target_properties(
    cm_fh: IO[str], targets: List[str], properties: List[str], indent: int = 0
):
    ind = spaces(indent)
    command_name = "set_target_properties"
    arguments_ind = spaces(indent + 1)

    prop_pairs = [(properties[i] + " " + properties[i + 1]) for i in range(0, len(properties), 2)]
    properties_str = f"\n{arguments_ind}" + f"\n{arguments_ind}".join(prop_pairs)

    if len(targets) == 1:
        targets_str = targets[0] + " "
    else:
        targets_str = (
            f"\n{arguments_ind}" + f"\n{arguments_ind}".join(targets) + f"\n{arguments_ind}"
        )

    cm_fh.write(f"{ind}{command_name}({targets_str}PROPERTIES{properties_str}\n{ind})\n")


def write_set_source_files_properties(
    cm_fh: IO[str], files: List[str], properties: List[str], indent: int = 0
):
    ind = spaces(indent)
    command_name = "set_source_files_properties"
    arguments_ind = spaces(indent + 1)

    prop_pairs = [(properties[i] + " " + properties[i + 1]) for i in range(0, len(properties), 2)]
    properties_str = f"\n{arguments_ind}" + f"\n{arguments_ind}".join(prop_pairs)

    if len(files) == 1:
        targets_str = files[0] + " "
    else:
        targets_str = f"\n{arguments_ind}" + f"\n{arguments_ind}".join(files) + f"\n{arguments_ind}"

    cm_fh.write(f"{ind}{command_name}({targets_str}PROPERTIES{properties_str}\n{ind})\n")


def write_target_sources(
    cm_fh: IO[str], target: str, sources: List[str], visibility: str = "PRIVATE", indent: int = 0
):
    command_name = "target_sources"
    header = f"{command_name}({target} {visibility}\n"
    write_list(cm_fh, sources, "", indent, footer=")", header=header)


def expand_project_requirements(scope: Scope, skip_message: bool = False) -> str:
    requirements = ""
    for requirement in scope.get("_REQUIREMENTS"):
        original_condition = simplify_condition(map_condition(requirement))
        inverted_requirement = simplify_condition(f"NOT ({map_condition(requirement)})")
        if not skip_message:
            message = f"""
{spaces(7)}message(NOTICE "Skipping the build as the condition \\"{original_condition}\\" is not met.")"""
        else:
            message = ""
        requirements += dedent(
            f"""\
                        if({inverted_requirement}){message}
                            return()
                        endif()
"""
        )
    return requirements


def write_extend_target(
    cm_fh: IO[str], target: str, scope: Scope, indent: int = 0, target_ref: str = None
):
    if target_ref is None:
        target_ref = target
    ind = spaces(indent)
    extend_qt_io_string = io.StringIO()
    write_sources_section(extend_qt_io_string, scope)
    extend_qt_string = extend_qt_io_string.getvalue()

    assert scope.total_condition, "Cannot write CONDITION when scope.condition is None"

    condition = map_to_cmake_condition(scope.total_condition)

    cmake_api_call = get_cmake_api_call("qt_extend_target")
    extend_scope = (
        f"\n{ind}{cmake_api_call}({target_ref} CONDITION"
        f" {condition}\n"
        f"{extend_qt_string}{ind})\n"
    )

    if not extend_qt_string:
        extend_scope = ""  # Nothing to report, so don't!

    cm_fh.write(extend_scope)

    io_string = io.StringIO()
    write_resources(io_string, target, scope, indent + 1, target_ref=target_ref)
    resource_string = io_string.getvalue()
    if len(resource_string) != 0:
        resource_string = resource_string.strip("\n").rstrip(f"\n{spaces(indent + 1)}")
        cm_fh.write(f"\n{spaces(indent)}if({condition})\n{resource_string}")
        cm_fh.write(f"\n{spaces(indent)}endif()\n")


def flatten_scopes(scope: Scope) -> List[Scope]:
    result = [scope]  # type: List[Scope]
    for c in scope.children:
        result += flatten_scopes(c)
    return result


def merge_scopes(scopes: List[Scope]) -> List[Scope]:
    result = []  # type: List[Scope]

    # Merge scopes with their parents:
    known_scopes = {}  # type: Dict[str, Scope]
    for scope in scopes:
        total_condition = scope.total_condition
        assert total_condition
        if total_condition == "OFF":
            # ignore this scope entirely!
            pass
        elif total_condition in known_scopes:
            known_scopes[total_condition].merge(scope)
        else:
            # Keep everything else:
            result.append(scope)
            known_scopes[total_condition] = scope

    return result


def write_simd_part(cm_fh: IO[str], target: str, scope: Scope, indent: int = 0):
    simd_options = [
        "sse2",
        "sse3",
        "ssse3",
        "sse4_1",
        "sse4_2",
        "aesni",
        "shani",
        "avx",
        "avx2",
        "avx512f",
        "avx512cd",
        "avx512er",
        "avx512pf",
        "avx512dq",
        "avx512bw",
        "avx512vl",
        "avx512ifma",
        "avx512vbmi",
        "f16c",
        "rdrnd",
        "neon",
        "mips_dsp",
        "mips_dspr2",
        "arch_haswell",
        "avx512common",
        "avx512core",
    ]

    simd_io_string = io.StringIO()

    condition = "ON"
    if scope.total_condition:
        condition = map_to_cmake_condition(scope.total_condition)

    if condition != "ON":
        indent += 1

    for simd in simd_options:
        SIMD = simd.upper()
        write_source_file_list(
            simd_io_string,
            scope,
            "SOURCES",
            [f"{SIMD}_HEADERS", f"{SIMD}_SOURCES", f"{SIMD}_C_SOURCES", f"{SIMD}_ASM"],
            indent=indent,
            header=f"{get_cmake_api_call('qt_add_simd_part')}({target} SIMD {simd}\n",
            footer=")\n",
        )

    simd_string = simd_io_string.getvalue()
    if simd_string:
        simd_string = simd_string.rstrip("\n")
        cond_start = ""
        cond_end = ""
        if condition != "ON":
            cond_start = f"{spaces(indent - 1)}if({condition})"
            cond_end = f"{spaces(indent - 1)}endif()"

        extend_scope = f"\n{cond_start}\n" f"{simd_string}" f"\n{cond_end}\n"
        cm_fh.write(extend_scope)


def write_reduce_relocations_part(cm_fh: IO[str], target: str, scope: Scope, indent: int = 0):
    ind = spaces(indent)
    dynlist_file = scope.get_files("QMAKE_DYNAMIC_LIST_FILE")
    if dynlist_file:
        dynlist_path = "${CMAKE_CURRENT_LIST_DIR}/" + dynlist_file[0]
        cm_fh.write(f"{ind}if(QT_FEATURE_reduce_relocations AND UNIX AND GCC)\n")
        ind = spaces(indent + 1)
        cm_fh.write(f"{ind}target_link_options({target} PRIVATE\n")
        cm_fh.write(f'{ind}                    "LINKER:--dynamic-list={dynlist_path}")\n')
        ind = spaces(indent)
        cm_fh.write(f"{ind}endif()\n")


def write_android_part(cm_fh: IO[str], target: str, scope: Scope, indent: int = 0):
    keys = [
        "ANDROID_BUNDLED_JAR_DEPENDENCIES",
        "ANDROID_LIB_DEPENDENCIES",
        "ANDROID_JAR_DEPENDENCIES",
        "ANDROID_LIB_DEPENDENCY_REPLACEMENTS",
        "ANDROID_BUNDLED_FILES",
        "ANDROID_PERMISSIONS",
        "ANDROID_PACKAGE_SOURCE_DIR",
    ]

    has_no_values = True
    for key in keys:
        value = scope.expand(key)
        if len(value) != 0:
            if has_no_values:
                if scope.condition:
                    cm_fh.write(f"\n{spaces(indent)}if(ANDROID AND ({scope.condition}))\n")
                else:
                    cm_fh.write(f"\n{spaces(indent)}if(ANDROID)\n")
                indent += 1
                has_no_values = False
            cm_fh.write(f"{spaces(indent)}set_property(TARGET {target} APPEND PROPERTY QT_{key}\n")
            write_list(cm_fh, value, "", indent + 1)
            cm_fh.write(f"{spaces(indent)})\n")
    indent -= 1

    if not has_no_values:
        cm_fh.write(f"{spaces(indent)}endif()\n")


def write_wayland_part(cm_fh: IO[str], target: str, scope: Scope, indent: int = 0):
    client_sources = scope.get_files("WAYLANDCLIENTSOURCES", use_vpath=True)
    server_sources = scope.get_files("WAYLANDSERVERSOURCES", use_vpath=True)
    if len(client_sources) == 0 and len(server_sources) == 0:
        return

    condition, indent = write_scope_condition_begin(cm_fh, scope, indent=indent)

    if len(client_sources) != 0:
        cm_fh.write(f"\n{spaces(indent)}qt6_generate_wayland_protocol_client_sources({target}\n")
        write_list(
            cm_fh, client_sources, "FILES", indent + 1, prefix="${CMAKE_CURRENT_SOURCE_DIR}/"
        )
        cm_fh.write(f"{spaces(indent)})\n")

    if len(server_sources) != 0:
        cm_fh.write(f"\n{spaces(indent)}qt6_generate_wayland_protocol_server_sources({target}\n")
        write_list(
            cm_fh, server_sources, "FILES", indent + 1, prefix="${CMAKE_CURRENT_SOURCE_DIR}/"
        )
        cm_fh.write(f"{spaces(indent)})\n")

    write_scope_condition_end(cm_fh, condition, indent=indent)


def write_scope_condition_begin(cm_fh: IO[str], scope: Scope, indent: int = 0) -> Tuple[str, int]:
    condition = "ON"
    if scope.total_condition:
        condition = map_to_cmake_condition(scope.total_condition)

    if condition != "ON":
        cm_fh.write(f"\n{spaces(indent)}if({condition})\n")
        indent += 1

    return condition, indent


def write_scope_condition_end(cm_fh: IO[str], condition: str, indent: int = 0) -> int:
    if condition != "ON":
        indent -= 1
        cm_fh.write(f"{spaces(indent)}endif()\n")
    return indent


def is_path_relative_ish(path: str) -> bool:
    if not os.path.isabs(path) and not path.startswith("$"):
        return True
    return False


def absolutify_path(path: str, base_dir: str = "${CMAKE_CURRENT_SOURCE_DIR}") -> str:
    if not path:
        return path
    if is_path_relative_ish(path):
        path = posixpath.join(base_dir, path)
    return path


def write_version_part(cm_fh: IO[str], target: str, scope: Scope, indent: int = 0):
    if scope.is_internal_qt_app:
        version_value = scope.get_string("VERSION")
        if version_value:
            version_value = re.sub(r"\$\${QT_VERSION\}", "${PROJECT_VERSION}", version_value)
        target_description = scope.expandString("QMAKE_TARGET_DESCRIPTION")

        if version_value or target_description:
            condition, indent = write_scope_condition_begin(cm_fh, scope, indent=indent)

            properties = []
            if version_value:
                properties.extend(["QT_TARGET_VERSION", f'"{version_value}"'])
            if target_description:
                properties.extend(["QT_TARGET_DESCRIPTION", f'"{target_description}"'])

            if properties:
                write_set_target_properties(cm_fh, [target], properties, indent=indent)

            write_scope_condition_end(cm_fh, condition, indent=indent)


def write_darwin_part(
    cm_fh: IO[str], target: str, scope: Scope, main_scope_target_name: str = "", indent: int = 0
):
    if scope.is_internal_qt_app:
        # Embed custom provided Info.plist file.
        info_plist = scope.expandString("QMAKE_INFO_PLIST")
        info_plist = absolutify_path(info_plist)

        icon_path = scope.expandString("ICON")
        icon_basename = ""

        new_output_name = None
        current_scope_output_name = scope.TARGET
        if current_scope_output_name != main_scope_target_name:
            new_output_name = current_scope_output_name

        if icon_path:
            icon_basename = os.path.basename(icon_path)

        if info_plist or icon_path or new_output_name:
            condition, indent = write_scope_condition_begin(cm_fh, scope, indent=indent)

            properties = []
            if info_plist:
                properties.extend(["MACOSX_BUNDLE_INFO_PLIST", f'"{info_plist}"'])
                properties.extend(["MACOSX_BUNDLE", "TRUE"])
            if icon_path:
                properties.extend(["MACOSX_BUNDLE_ICON_FILE", f'"{icon_basename}"'])

            if new_output_name:
                properties.extend(["OUTPUT_NAME", f'"{new_output_name}"'])

            if properties:
                write_set_target_properties(cm_fh, [target], properties, indent=indent)
                if icon_path:
                    source_properties = ["MACOSX_PACKAGE_LOCATION", "Resources"]
                    write_set_source_files_properties(
                        cm_fh, [icon_path], source_properties, indent=indent
                    )
                    write_target_sources(cm_fh, target, [icon_path], indent=indent)

            write_scope_condition_end(cm_fh, condition, indent=indent)


def write_windows_part(cm_fh: IO[str], target: str, scope: Scope, indent: int = 0):
    if scope.is_internal_qt_app:
        # Handle CONFIG += console assignments.
        is_console = "console" in scope.get("CONFIG")
        rc_file = scope.expandString("RC_FILE")
        rc_file = absolutify_path(rc_file)
        rc_icons = scope.expandString("RC_ICONS")
        rc_icons = absolutify_path(rc_icons)

        if is_console or rc_file or rc_icons:
            condition, indent = write_scope_condition_begin(cm_fh, scope, indent=indent)

            properties = []
            if is_console:
                properties.extend(["WIN32_EXECUTABLE", "FALSE"])

            if rc_file:
                properties.extend(["QT_TARGET_WINDOWS_RC_FILE", f'"{rc_file}"'])

            if rc_icons:
                properties.extend(["QT_TARGET_RC_ICONS", f'"{rc_icons}"'])

            if properties:
                write_set_target_properties(cm_fh, [target], properties, indent=indent)

            write_scope_condition_end(cm_fh, condition, indent=indent)


def write_aux_qml_file_install_call(cm_fh: IO[str], file_list: List[str], indent: int = 0):
    cm_fh.write(f"\n{spaces(indent)}qt_copy_or_install(\n")
    write_list(cm_fh, file_list, "FILES", indent + 1)

    destination_option = 'DESTINATION "${__aux_qml_files_install_dir}"'
    cm_fh.write(f"{spaces(indent + 1)}{destination_option})\n")


def write_aux_qml_path_setup(cm_fh: IO[str], base_dir: str, indent: int = 0):
    path_join_args = f'__aux_qml_files_install_dir "${{__aux_qml_files_install_base}}" "{base_dir}"'
    cm_fh.write(f"\n{spaces(indent)}qt_path_join({path_join_args})\n")


def write_aux_qml_files_part(cm_fh: IO[str], target: str, scope: Scope, indent: int = 0):
    aux_files = scope.get_files("AUX_QML_FILES")
    if aux_files and isinstance(aux_files, list):
        aux_files_per_dir = defaultdict(list)
        aux_files_globs = []

        # Handle globs differently from regular paths.
        # For regular paths, group by base dir. Each base dir will get
        # its own install call.
        for path in aux_files:
            if "*" in path:
                aux_files_globs.append(path)
            else:
                base_dir = os.path.dirname(path)
                aux_files_per_dir[base_dir].append(path)

        condition, indent = write_scope_condition_begin(cm_fh, scope, indent=indent)

        # Extract the location of $prefix/qml, where we want to install
        # files.
        get_prop_args = f"__aux_qml_files_install_base {target} QT_QML_MODULE_INSTALL_DIR"
        cm_fh.write(f"{spaces(indent)}get_target_property({get_prop_args})\n")

        # Handle glob installs.
        for path in aux_files_globs:
            cm_fh.write(
                f"""
{spaces(indent)}file(GLOB_RECURSE __aux_qml_glob_files
{spaces(indent + 1)}RELATIVE "${{CMAKE_CURRENT_SOURCE_DIR}}"
{spaces(indent + 1)}"{path}")"""
            )
            file_list = ["${__aux_qml_glob_files}"]

            # Extract base dir. Hopes that the globs only appear in the
            # file name part.
            base_dir = os.path.dirname(path)
            write_aux_qml_path_setup(cm_fh, base_dir, indent=indent)
            write_aux_qml_file_install_call(cm_fh, file_list, indent=indent)

        # Handle regular per base-dir installs.
        for base_dir in aux_files_per_dir:
            file_list = aux_files_per_dir[base_dir]
            write_aux_qml_path_setup(cm_fh, base_dir, indent=indent)
            write_aux_qml_file_install_call(cm_fh, file_list, indent=indent)
        write_scope_condition_end(cm_fh, condition, indent=indent)


def handle_source_subtractions(scopes: List[Scope]):
    """
    Handles source subtractions like SOURCES -= painting/qdrawhelper.cpp
    by creating a new scope with a new condition containing all addition
    and subtraction conditions.

    Algorithm is as follows:
    - Go through each scope and find files in SOURCES starting with "-"
    - Save that file and the scope condition in modified_sources dict.
    - Remove the file from the found scope (optionally remove the
      NO_PCH_SOURCES entry for that file as well).
    - Go through each file in modified_sources dict.
    - Find scopes where the file is added, remove the file from that
      scope and save the condition.
    - Create a new scope just for that file with a new simplified
      condition that takes all the other conditions into account.
    """

    def remove_file_from_operation(
        scope: Scope, ops_key: str, file: str, op_type: Type[Operation]
    ) -> bool:
        """
        Remove a source file from an operation in a scope.
        Example: remove foo.cpp from any operations that have
                 ops_key="SOURCES" in "scope", where the operation is of
                 type "op_type".

        The implementation is very rudimentary and might not work in
        all cases.

        Returns True if a file was found and removed in any operation.
        """
        file_removed = False
        ops = scope._operations.get(ops_key, list())
        for op in ops:
            if not isinstance(op, op_type):
                continue
            if file in op._value:
                op._value.remove(file)
                file_removed = True
        for include_child_scope in scope._included_children:
            file_removed = file_removed or remove_file_from_operation(
                include_child_scope, ops_key, file, op_type
            )
        return file_removed

    def join_all_conditions(set_of_alternatives: Set[str]):
        final_str = ""
        if set_of_alternatives:
            alternatives = [f"({alternative})" for alternative in set_of_alternatives]
            final_str = " OR ".join(sorted(alternatives))
        return final_str

    modified_sources: Dict[str, Dict[str, Union[Set[str], bool]]] = {}

    new_scopes = []
    top_most_scope = scopes[0]

    for scope in scopes:
        sources = scope.get_files("SOURCES")
        for file in sources:
            # Find subtractions.
            if file.startswith("-"):
                file_without_minus = file[1:]

                if file_without_minus not in modified_sources:
                    modified_sources[file_without_minus] = {}

                subtractions = modified_sources[file_without_minus].get("subtractions", set())
                assert isinstance(subtractions, set)

                # Add the condition to the set of conditions and remove
                # the file subtraction from the processed scope, which
                # will be later re-added in a new scope.
                if scope.condition:
                    assert scope.total_condition
                    subtractions.add(scope.total_condition)
                remove_file_from_operation(scope, "SOURCES", file_without_minus, RemoveOperation)
                if subtractions:
                    modified_sources[file_without_minus]["subtractions"] = subtractions

                # In case if the source is also listed in a
                # NO_PCH_SOURCES operation, remove it from there as
                # well, and add it back later.
                no_pch_source_removed = remove_file_from_operation(
                    scope, "NO_PCH_SOURCES", file_without_minus, AddOperation
                )
                if no_pch_source_removed:
                    modified_sources[file_without_minus]["add_to_no_pch_sources"] = True

    for modified_source in modified_sources:
        additions = modified_sources[modified_source].get("additions", set())
        assert isinstance(additions, set), f"Additions must be a set, got {additions} instead."
        subtractions = modified_sources[modified_source].get("subtractions", set())
        assert isinstance(
            subtractions, set
        ), f"Subtractions must be a set, got {additions} instead."
        add_to_no_pch_sources = modified_sources[modified_source].get(
            "add_to_no_pch_sources", False
        )

        for scope in scopes:
            sources = scope.get_files("SOURCES")
            if modified_source in sources:
                # Remove the source file from any addition operations
                # that mention it.
                remove_file_from_operation(scope, "SOURCES", modified_source, AddOperation)
                if scope.total_condition:
                    additions.add(scope.total_condition)

        # Construct a condition that takes into account all addition
        # and subtraction conditions.
        addition_str = join_all_conditions(additions)
        if addition_str:
            addition_str = f"({addition_str})"
        subtraction_str = join_all_conditions(subtractions)
        if subtraction_str:
            subtraction_str = f"NOT ({subtraction_str})"

        condition_str = addition_str
        if condition_str and subtraction_str:
            condition_str += " AND "
        condition_str += subtraction_str
        condition_simplified = simplify_condition(condition_str)

        # Create a new scope with that condition and add the source
        # operations.
        new_scope = Scope(
            parent_scope=top_most_scope,
            qmake_file=top_most_scope.file,
            condition=condition_simplified,
            base_dir=top_most_scope.basedir,
        )
        new_scope.total_condition = condition_simplified
        new_scope._append_operation("SOURCES", AddOperation([modified_source]))
        if add_to_no_pch_sources:
            new_scope._append_operation("NO_PCH_SOURCES", AddOperation([modified_source]))

        new_scopes.append(new_scope)

    # Add all the newly created scopes.
    scopes += new_scopes


def write_main_part(
    cm_fh: IO[str],
    name: str,
    typename: str,
    cmake_function: str,
    scope: Scope,
    *,
    extra_lines: Optional[List[str]] = None,
    indent: int = 0,
    extra_keys: List[str],
    **kwargs: Any,
):
    # Evaluate total condition of all scopes:
    if extra_lines is None:
        extra_lines = []
    recursive_evaluate_scope(scope)

    if "exceptions" in scope.get("CONFIG"):
        extra_lines.append("EXCEPTIONS")

    # Get a flat list of all scopes but the main one:
    scopes = flatten_scopes(scope)
    # total_scopes = len(scopes)
    # Merge scopes based on their conditions:
    scopes = merge_scopes(scopes)

    # Handle SOURCES -= foo calls, and merge scopes one more time
    # because there might have been several files removed with the same
    # scope condition.
    handle_source_subtractions(scopes)
    scopes = merge_scopes(scopes)

    assert len(scopes)
    assert scopes[0].total_condition == "ON"

    scopes[0].reset_visited_keys()
    for k in extra_keys:
        scopes[0].get(k)

    # Now write out the scopes:
    write_header(cm_fh, name, typename, indent=indent)

    # collect all testdata and insert globbing commands
    has_test_data = False
    if typename == "Test":
        cm_fh.write(f"{spaces(indent)}if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)\n")
        cm_fh.write(f"{spaces(indent+1)}cmake_minimum_required(VERSION 3.16)\n")
        cm_fh.write(f"{spaces(indent+1)}project({name} LANGUAGES C CXX ASM)\n")
        cm_fh.write(
            f"{spaces(indent+1)}find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)\n"
        )
        cm_fh.write(f"{spaces(indent)}endif()\n\n")

        test_data = scope.expand("TESTDATA")
        if test_data:
            has_test_data = True
            cm_fh.write("# Collect test data\n")
            for data in test_data:
                if "*" in data:
                    cm_fh.write(
                        dedent(
                            f"""\
                        {spaces(indent)}file(GLOB_RECURSE test_data_glob
                        {spaces(indent+1)}RELATIVE ${{CMAKE_CURRENT_SOURCE_DIR}}
                        {spaces(indent+1)}{data})
                        """
                        )
                    )
                    cm_fh.write(f"{spaces(indent)}list(APPEND test_data ${{test_data_glob}})\n")
                else:
                    cm_fh.write(f'{spaces(indent)}list(APPEND test_data "{data}")\n')
            cm_fh.write("\n")

    target_ref = name
    if typename == "Tool":
        target_ref = "${target_name}"
        cm_fh.write(f"{spaces(indent)}qt_get_tool_target_name(target_name {name})\n")

    # Check for DESTDIR override
    destdir = scope.get_string("DESTDIR")
    if destdir:
        already_added = False
        for line in extra_lines:
            if line.startswith("OUTPUT_DIRECTORY"):
                already_added = True
                break
        if not already_added:
            destdir = replace_path_constants(destdir, scope)
            extra_lines.append(f'OUTPUT_DIRECTORY "{destdir}"')

    cm_fh.write(f"{spaces(indent)}{cmake_function}({target_ref}\n")
    for extra_line in extra_lines:
        cm_fh.write(f"{spaces(indent)}    {extra_line}\n")

    write_sources_section(cm_fh, scopes[0], indent=indent, **kwargs)

    if has_test_data:
        cm_fh.write(f"{spaces(indent)}    TESTDATA ${{test_data}}\n")
    # Footer:
    cm_fh.write(f"{spaces(indent)})\n")

    if typename == "Tool":
        cm_fh.write(f"{spaces(indent)}qt_internal_return_unless_building_tools()\n")

    write_resources(cm_fh, name, scope, indent, target_ref=target_ref)

    write_statecharts(cm_fh, name, scope, indent)

    write_qlalrsources(cm_fh, name, scope, indent)

    write_repc_files(cm_fh, name, scope, indent)

    write_simd_part(cm_fh, name, scope, indent)

    write_reduce_relocations_part(cm_fh, name, scope, indent)

    write_android_part(cm_fh, name, scopes[0], indent)

    write_wayland_part(cm_fh, name, scopes[0], indent)

    write_windows_part(cm_fh, name, scopes[0], indent)

    write_darwin_part(cm_fh, name, scopes[0], main_scope_target_name=name, indent=indent)

    write_version_part(cm_fh, name, scopes[0], indent)

    write_aux_qml_files_part(cm_fh, name, scopes[0], indent)

    if "warn_off" in scope.get("CONFIG"):
        write_generic_cmake_command(cm_fh, "qt_disable_warnings", [name])

    if "hide_symbols" in scope.get("CONFIG"):
        write_generic_cmake_command(cm_fh, "qt_set_symbol_visibility_hidden", [name])

    ignored_keys_report = write_ignored_keys(scopes[0], spaces(indent))
    if ignored_keys_report:
        cm_fh.write(ignored_keys_report)

    # Scopes:
    if len(scopes) == 1:
        return

    write_scope_header(cm_fh, indent=indent)

    for c in scopes[1:]:
        c.reset_visited_keys()
        write_android_part(cm_fh, name, c, indent=indent)
        write_wayland_part(cm_fh, name, c, indent=indent)
        write_windows_part(cm_fh, name, c, indent=indent)
        write_darwin_part(cm_fh, name, c, main_scope_target_name=name, indent=indent)
        write_version_part(cm_fh, name, c, indent=indent)
        write_aux_qml_files_part(cm_fh, name, c, indent=indent)
        write_extend_target(cm_fh, name, c, target_ref=target_ref, indent=indent)
        write_simd_part(cm_fh, name, c, indent=indent)

        ignored_keys_report = write_ignored_keys(c, spaces(indent))
        if ignored_keys_report:
            cm_fh.write(ignored_keys_report)


def write_3rdparty_library(cm_fh: IO[str], scope: Scope, *, indent: int = 0) -> str:
    # Remove default QT libs.
    scope._append_operation("QT", RemoveOperation(["core", "gui"]))

    target_name = re.sub(r"^qt", "", scope.TARGET)
    target_name = target_name.replace("-", "_")
    qmake_lib_name = target_name

    # Capitalize the first letter for a nicer name.
    target_name = target_name.title()

    # Prefix with Bundled, to avoid possible duplicate target names
    # e.g. "BundledFreetype" instead of "freetype".
    target_name = f"Bundled{target_name}"

    if "dll" in scope.get("CONFIG"):
        library_type = "SHARED"
    else:
        library_type = "STATIC"

    extra_lines = [f"QMAKE_LIB_NAME {qmake_lib_name}"]

    if library_type:
        extra_lines.append(library_type)

    if "installed" in scope.get("CONFIG"):
        extra_lines.append("INSTALL")

    write_main_part(
        cm_fh,
        target_name,
        "Generic Library",
        get_cmake_api_call("qt_add_3rdparty_library"),
        scope,
        extra_lines=extra_lines,
        indent=indent,
        known_libraries={},
        extra_keys=[],
    )

    return target_name


def write_generic_library(cm_fh: IO[str], scope: Scope, *, indent: int = 0) -> str:

    target_name = scope.TARGET

    library_type = ""

    if "dll" in scope.get("CONFIG"):
        library_type = "SHARED"

    is_plugin = False
    if "plugin" in scope.get("CONFIG"):
        library_type = "MODULE"
        is_plugin = True

    # static after plugin in order to handle static library plugins
    if "static" in scope.get("CONFIG"):
        library_type = "STATIC"

    extra_lines = []

    if library_type:
        extra_lines.append(library_type)

    target_path = scope.expandString("target.path")
    target_path = replace_path_constants(target_path, scope)
    if target_path:
        extra_lines.append(f'INSTALL_DIRECTORY "{target_path}"')

    write_main_part(
        cm_fh,
        target_name,
        "Generic Library",
        get_cmake_api_call("qt_add_cmake_library"),
        scope,
        extra_lines=extra_lines,
        indent=indent,
        known_libraries={},
        extra_keys=[],
    )

    if is_plugin:
        # Plugins need to be able to run auto moc
        cm_fh.write(f"\nqt_autogen_tools_initial_setup({target_name})\n")

        if library_type == "STATIC":
            cm_fh.write(f"\ntarget_compile_definitions({target_name} PRIVATE QT_STATICPLUGIN)\n")

    return target_name


def forward_target_info(scope: Scope, extra: List[str], skip: Optional[Dict[str, bool]] = None):
    s = scope.get_string("QMAKE_TARGET_PRODUCT")
    if s:
        extra.append(f'TARGET_PRODUCT "{s}"')
    s = scope.get_string("QMAKE_TARGET_DESCRIPTION")
    if s and (not skip or "QMAKE_TARGET_DESCRIPTION" not in skip):
        extra.append(f'TARGET_DESCRIPTION "{s}"')
    s = scope.get_string("QMAKE_TARGET_COMPANY")
    if s:
        extra.append(f'TARGET_COMPANY "{s}"')
    s = scope.get_string("QMAKE_TARGET_COPYRIGHT")
    if s:
        extra.append(f'TARGET_COPYRIGHT "{s}"')


def write_module(cm_fh: IO[str], scope: Scope, *, indent: int = 0) -> str:
    # e.g. QtCore
    qt_module_name = scope.TARGET
    if not qt_module_name.startswith("Qt"):
        print(f"XXXXXX Module name {qt_module_name} does not start with Qt!")

    extra = []

    # A module should be static when 'static' is in CONFIG
    # or when option(host_build) is used, as described in qt_module.prf.
    is_static = "static" in scope.get("CONFIG") or "host_build" in scope.get("_OPTION")

    is_public_module = True

    # CMake target name as passed to qt_internal_add_module()
    # e.g. Core
    cmake_target_name = qt_module_name[2:]

    # MODULE is used for the name of the generated .pri file.
    # If MODULE is not explicitly set, qmake computes its value in
    # mkspecs/features/qt_build_config.prf
    module_name_for_pri = scope.expandString("MODULE")
    if not module_name_for_pri:
        module_name_for_pri_as_qmake_computes_it = scope.file[:-4]
        module_name_for_pri = module_name_for_pri_as_qmake_computes_it

    # Given 'qt_internal_add_module(Core)', computes 'core'.
    module_name_for_pri_as_cmake_computes_it = cmake_target_name.lower()

    if module_name_for_pri != module_name_for_pri_as_cmake_computes_it:
        extra.append(f"CONFIG_MODULE_NAME {module_name_for_pri}")

    if is_static:
        extra.append("STATIC")
    if "internal_module" in scope.get("CONFIG"):
        is_public_module = False
        cmake_target_name += "Private"  # Assume all internal modules have the 'Private' suffix
        extra.append("INTERNAL_MODULE")
    if "no_module_headers" in scope.get("CONFIG"):
        extra.append("NO_MODULE_HEADERS")
    if "minimal_syncqt" in scope.get("CONFIG"):
        extra.append("NO_SYNC_QT")
    if "no_private_module" in scope.get("CONFIG"):
        extra.append("NO_PRIVATE_MODULE")
    else:
        scope._has_private_module = True
    if "header_module" in scope.get("CONFIG"):
        extra.append("HEADER_MODULE")
    if not("metatypes" in scope.get("CONFIG") or "qmltypes" in scope.get("CONFIG")):
        extra.append("NO_GENERATE_METATYPES")

    module_config = scope.get("MODULE_CONFIG")
    if len(module_config):
        extra.append(f'QMAKE_MODULE_CONFIG {" ".join(module_config)}')

    module_plugin_types = scope.get_files("MODULE_PLUGIN_TYPES")
    if module_plugin_types:
        extra.append(f"PLUGIN_TYPES {' '.join(module_plugin_types)}")

    scope._is_public_module = is_public_module

    forward_target_info(scope, extra)
    write_main_part(
        cm_fh,
        cmake_target_name,
        "Module",
        f"{get_cmake_api_call('qt_add_module')}",
        scope,
        extra_lines=extra,
        indent=indent,
        known_libraries={},
        extra_keys=[],
    )

    if "qt_tracepoints" in scope.get("CONFIG"):
        tracepoints = scope.get_files("TRACEPOINT_PROVIDER")
        create_trace_points = get_cmake_api_call("qt_create_tracepoints")
        cm_fh.write(
            f"\n\n{spaces(indent)}{create_trace_points}({cmake_target_name} {' '.join(tracepoints)})\n"
        )

    return cmake_target_name


def write_tool(cm_fh: IO[str], scope: Scope, *, indent: int = 0) -> Tuple[str, str]:
    tool_name = scope.TARGET

    if "force_bootstrap" in scope.get("CONFIG"):
        extra = ["BOOTSTRAP"]

        # Remove default QT libs.
        scope._append_operation("QT", RemoveOperation(["core", "gui"]))
    else:
        extra = []

    forward_target_info(scope, extra)

    write_main_part(
        cm_fh,
        tool_name,
        "Tool",
        get_cmake_api_call("qt_add_tool"),
        scope,
        indent=indent,
        known_libraries={"Qt::Core"},
        extra_lines=extra,
        extra_keys=["CONFIG"],
    )

    return tool_name, "${target_name}"


def write_qt_app(cm_fh: IO[str], scope: Scope, *, indent: int = 0) -> str:
    app_name = scope.TARGET

    extra: List[str] = []

    target_info_skip = {}
    target_info_skip["QMAKE_TARGET_DESCRIPTION"] = True
    forward_target_info(scope, extra, target_info_skip)

    write_main_part(
        cm_fh,
        app_name,
        "App",
        get_cmake_api_call("qt_internal_add_app"),
        scope,
        indent=indent,
        known_libraries={"Qt::Core"},
        extra_lines=extra,
        extra_keys=["CONFIG"],
    )

    return app_name


def write_test(cm_fh: IO[str], scope: Scope, gui: bool = False, *, indent: int = 0) -> str:
    test_name = scope.TARGET
    assert test_name

    extra = ["GUI"] if gui else []
    libraries = {"Qt::Core", "Qt::Test"}

    if "qmltestcase" in scope.get("CONFIG"):
        libraries.add("Qt::QmlTest")
        extra.append("QMLTEST")
        importpath = scope.expand("IMPORTPATH")
        if importpath:
            extra.append("QML_IMPORTPATH")
            for path in importpath:
                extra.append(f'    "{path}"')

    target_original = scope.TARGET_ORIGINAL
    if target_original and target_original.startswith("../"):
        extra.append('OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"')

    requires_content = expand_project_requirements(scope, skip_message=True)
    if requires_content:
        requires_content += "\n"
        cm_fh.write(requires_content)

    write_main_part(
        cm_fh,
        test_name,
        "Test",
        get_cmake_api_call("qt_add_test"),
        scope,
        indent=indent,
        known_libraries=libraries,
        extra_lines=extra,
        extra_keys=[],
    )

    return test_name


def write_binary(cm_fh: IO[str], scope: Scope, gui: bool = False, *, indent: int = 0) -> str:
    binary_name = scope.TARGET
    assert binary_name

    is_benchmark = is_benchmark_project(scope.file_absolute_path)
    is_manual_test = is_manual_test_project(scope.file_absolute_path)

    is_qt_test_helper = "qt_test_helper" in scope.get("_LOADED")

    extra = ["GUI"] if gui and not is_qt_test_helper else []
    cmake_function_call = get_cmake_api_call("qt_add_executable")
    extra_keys: List[str] = []

    if is_qt_test_helper:
        binary_name += "_helper"
        cmake_function_call = get_cmake_api_call("qt_add_test_helper")

    if is_benchmark:
        cmake_function_call = get_cmake_api_call("qt_add_benchmark")
    elif is_manual_test:
        cmake_function_call = get_cmake_api_call("qt_add_manual_test")
    else:
        extra_keys = ["target.path", "INSTALLS"]
        target_path = scope.get_string("target.path")
        if target_path:
            target_path = replace_path_constants(target_path, scope)
            if not scope.get("DESTDIR"):
                extra.append(f'OUTPUT_DIRECTORY "{target_path}"')
            if "target" in scope.get("INSTALLS"):
                extra.append(f'INSTALL_DIRECTORY "{target_path}"')

    write_main_part(
        cm_fh,
        binary_name,
        "Binary",
        cmake_function_call,
        scope,
        extra_lines=extra,
        indent=indent,
        known_libraries={"Qt::Core"},
        extra_keys=extra_keys,
    )

    return binary_name


def write_find_package_section(
    cm_fh: IO[str],
    public_libs: List[str],
    private_libs: List[str],
    *,
    indent: int = 0,
    is_required: bool = True,
    end_with_extra_newline: bool = True,
    qt_package_name: str = "Qt6",
):
    packages = []  # type: List[LibraryMapping]
    all_libs = public_libs + private_libs

    for one_lib in all_libs:
        info = find_library_info_for_target(one_lib)
        if info and info not in packages:
            packages.append(info)

    qt_components: List[str] = []
    for p in filter(LibraryMapping.is_qt, packages):
        if p.components is not None:
            qt_components += p.components
    if qt_components:
        if "Core" in qt_components:
            qt_components.remove("Core")
        qt_components = sorted(qt_components)
        qt_package = LibraryMapping("unknown", qt_package_name, "unknown", components=qt_components)
        if is_required:
            qt_package.extra = ["REQUIRED"]
        cm_fh.write(
            generate_find_package_info(
                qt_package,
                use_qt_find_package=False,
                remove_REQUIRED_from_extra=False,
                components_required=is_required,
                indent=indent,
            )
        )

    for p in itertools.filterfalse(LibraryMapping.is_qt, packages):
        cm_fh.write(generate_find_package_info(p, use_qt_find_package=False, indent=indent))

    if packages and end_with_extra_newline:
        cm_fh.write("\n")


def write_jar(cm_fh: IO[str], scope: Scope, *, indent: int = 0) -> str:

    target = scope.TARGET

    install_dir = scope.expandString("target.path")
    if not install_dir:
        raise RuntimeError("Could not locate jar install path")
    install_dir = install_dir.replace("$$[QT_INSTALL_PREFIX]/", "")

    android_sdk_jar = "${QT_ANDROID_JAR}"
    android_api_level = scope.get_string("API_VERSION")
    if android_api_level:
        cm_fh.write(
            f'{spaces(indent)}qt_get_android_sdk_jar_for_api("{android_api_level}" android_sdk)\n\n'
        )
        android_sdk_jar = "${android_sdk}"

    write_source_file_list(
        cm_fh, scope, "", ["JAVASOURCES"], indent=indent, header="set(java_sources\n", footer=")\n"
    )

    cm_fh.write(f"{spaces(indent)}qt_internal_add_jar({target}\n")
    cm_fh.write(f"{spaces(indent+1)}INCLUDE_JARS {android_sdk_jar}\n")
    cm_fh.write(f"{spaces(indent+1)}SOURCES ${{java_sources}}\n")
    cm_fh.write(f'{spaces(indent+1)}OUTPUT_DIR "${{QT_BUILD_DIR}}/{install_dir}"\n')
    cm_fh.write(f"{spaces(indent)})\n\n")

    cm_fh.write(f"{spaces(indent)}install_jar({target}\n")
    cm_fh.write(f"{spaces(indent+1)}DESTINATION {install_dir}\n")
    cm_fh.write(f"{spaces(indent+1)}COMPONENT Devel\n")
    cm_fh.write(f"{spaces(indent)})\n\n")

    return target


def get_win32_and_mac_bundle_properties(scope: Scope) -> tuple:
    config = scope.get("CONFIG")
    win32 = all(val not in config for val in ["cmdline", "console"])
    mac_bundle = all(val not in config for val in ["cmdline", "-app_bundle"])

    return win32, mac_bundle


def write_win32_and_mac_bundle_properties(
    cm_fh: IO[str], scope: Scope, target: str, *, handling_first_scope=False, indent: int = 0
):
    win32, mac_bundle = get_win32_and_mac_bundle_properties(scope)

    true_value = "TRUE"
    false_value = "FALSE"

    properties_mapping = {
        "WIN32_EXECUTABLE": true_value if win32 else false_value,
        "MACOSX_BUNDLE": true_value if mac_bundle else false_value,
    }

    properties = []

    # Always write the properties for the first scope.
    # For conditional scopes, only write them if the value is different
    # from the default value (aka different from TRUE).
    # This is a heurestic that should cover 90% of the example projects
    # without creating excess noise of setting the properties in every
    # single scope.
    for name, value in properties_mapping.items():
        if not handling_first_scope and value != true_value:
            properties.extend([name, value])

    if properties:
        write_set_target_properties(cm_fh, [target], properties, indent=indent)


def is_qtquick_source_file(filename: str):
    return filename.endswith(".qml") or filename.endswith(".js") or filename.endswith(".mjs")


def looks_like_qml_resource(resource: QtResource):
    if resource.generated or "*" in resource.name:
        return False
    for f in resource.files:
        if is_qtquick_source_file(f):
            return True
    return False


def find_qml_resource(resources: List[QtResource]):
    """Return the resource object that's most likely the one that should be used for
    qt_add_qml_module. Return None if there's no such resource."""
    return next(filter(looks_like_qml_resource, resources), None)


def write_example(
    cm_fh: IO[str],
    scope: Scope,
    gui: bool = False,
    *,
    indent: int = 0,
    is_plugin: bool = False,
    is_user_project: bool = False,
) -> str:
    binary_name = scope.TARGET
    assert binary_name
    config = scope.get("CONFIG")
    is_qml_plugin = ("qml" in scope.get("QT")) or "qmltypes" in config

    if not is_user_project:
        example_install_dir = scope.expandString("target.path")
        if not example_install_dir:
            example_install_dir = "${INSTALL_EXAMPLESDIR}"
        example_install_dir = example_install_dir.replace(
            "$$[QT_INSTALL_EXAMPLES]", "${INSTALL_EXAMPLESDIR}"
        )

    project_version = scope.get_string("VERSION", "1.0")
    cm_fh.write(
        f"cmake_minimum_required(VERSION {cmake_version_string})\n"
        f"project({binary_name} VERSION {project_version} LANGUAGES CXX)\n\n"
        "set(CMAKE_INCLUDE_CURRENT_DIR ON)\n\n"
        "set(CMAKE_AUTOMOC ON)\n"
    )
    if scope.get_files("FORMS"):
        cm_fh.write("set(CMAKE_AUTOUIC ON)\n")
    cm_fh.write("\n")
    if not is_user_project:
        cm_fh.write(
            "if(NOT DEFINED INSTALL_EXAMPLESDIR)\n"
            '    set(INSTALL_EXAMPLESDIR "examples")\n'
            "endif()\n\n"
            f'set(INSTALL_EXAMPLEDIR "{example_install_dir}")\n\n'
        )

    recursive_evaluate_scope(scope)

    # Get a flat list of all scopes but the main one:
    scopes = flatten_scopes(scope)
    # Merge scopes based on their conditions:
    scopes = merge_scopes(scopes)
    # Handle SOURCES -= foo calls, and merge scopes one more time
    # because there might have been several files removed with the same
    # scope condition.
    handle_source_subtractions(scopes)
    scopes = merge_scopes(scopes)

    # Write find_package call for Qt5/Qt6 and make it available as package QT.
    cm_fh.write("find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)\n")

    # Write find_package calls for required packages.
    # We consider packages as required if they appear at the top-level scope.
    (public_libs, private_libs) = extract_cmake_libraries(scope, is_example=True)
    write_find_package_section(
        cm_fh,
        public_libs,
        private_libs,
        indent=indent,
        end_with_extra_newline=False,
        qt_package_name="Qt${QT_VERSION_MAJOR}",
    )

    # Write find_package calls for optional packages.
    # We consider packages inside scopes other than the top-level one as optional.
    optional_public_libs: List[str] = []
    optional_private_libs: List[str] = []
    handling_first_scope = True
    for inner_scope in scopes:
        if handling_first_scope:
            handling_first_scope = False
            continue
        (public_libs, private_libs) = extract_cmake_libraries(inner_scope, is_example=True)
        optional_public_libs += public_libs
        optional_private_libs += private_libs
    write_find_package_section(
        cm_fh,
        optional_public_libs,
        optional_private_libs,
        indent=indent,
        is_required=False,
        end_with_extra_newline=False,
        qt_package_name="Qt${QT_VERSION_MAJOR}",
    )

    cm_fh.write("\n")

    (resources, standalone_qtquick_compiler_skipped_files) = extract_resources(binary_name, scope)
    qml_resource = find_qml_resource(resources) if is_qml_plugin else None

    add_target = ""

    if is_plugin:
        if is_qml_plugin:
            extra_args = [f"PLUGIN_TARGET {binary_name}"]
            io_string = io.StringIO()
            write_qml_module(
                io_string,
                binary_name,
                scope,
                scopes,
                indent=indent,
                resource=qml_resource,
                extra_add_qml_module_args=extra_args,
            )
            add_target += io_string.getvalue()
        else:
            add_target = f"qt_add_plugin({binary_name}"
            if "static" in scope.get("CONFIG"):
                add_target += " STATIC"
            add_target += ")\n"
        add_target += f"target_sources({binary_name} PRIVATE"
    else:
        add_target = f"qt_add_executable({binary_name}"

        property_win32, property_mac_bundle = get_win32_and_mac_bundle_properties(scope)

        if property_win32:
            add_target += " " + "WIN32"
        if property_mac_bundle:
            add_target += " " + "MACOSX_BUNDLE"

    write_all_source_file_lists(cm_fh, scope, add_target, indent=0)
    cm_fh.write(")\n")

    if is_qml_plugin and not is_plugin:
        write_qml_module(cm_fh, binary_name, scope, scopes, indent=indent, resource=qml_resource)

    handling_first_scope = True

    for scope in scopes:
        # write wayland already has condition scope handling
        write_wayland_part(cm_fh, binary_name, scope, indent=0)

        # The following options do not
        io_string = io.StringIO()
        condition_str = ""
        condition = "ON"
        if scope.total_condition:
            condition = map_to_cmake_condition(scope.total_condition)

        if condition != "ON":
            condition_str = f"\n{spaces(indent)}if({condition})\n"
            indent += 1

        if not handling_first_scope:
            target_sources = f"target_sources({binary_name} PUBLIC"
            write_all_source_file_lists(
                io_string, scope, target_sources, indent=indent, footer=")\n"
            )

        write_win32_and_mac_bundle_properties(
            io_string, scope, binary_name, handling_first_scope=handling_first_scope, indent=indent
        )

        write_include_paths(
            io_string,
            scope,
            f"target_include_directories({binary_name} PUBLIC",
            indent=indent,
            footer=")\n",
        )
        write_defines(
            io_string,
            scope,
            f"target_compile_definitions({binary_name} PUBLIC",
            indent=indent,
            footer=")\n",
        )

        (scope_public_libs, scope_private_libs) = extract_cmake_libraries(scope, is_example=True)

        write_list(
            io_string,
            scope_private_libs,
            "",
            indent=indent,
            header=f"target_link_libraries({binary_name} PRIVATE\n",
            footer=")\n",
        )
        write_list(
            io_string,
            scope_public_libs,
            "",
            indent=indent,
            header=f"target_link_libraries({binary_name} PUBLIC\n",
            footer=")\n",
        )
        write_compile_options(
            io_string, scope, f"target_compile_options({binary_name}", indent=indent, footer=")\n"
        )

        (resources, standalone_qtquick_compiler_skipped_files) = extract_resources(
            binary_name, scope
        )

        # Remove the QML resource, because we've handled it in write_qml_module.
        if qml_resource is not None:
            resources = list(filter(lambda r: r.name != qml_resource.name, resources))

        write_resources(
            io_string,
            binary_name,
            scope,
            indent=indent,
            is_example=True,
            resources=resources,
            skipped_standalone_files=standalone_qtquick_compiler_skipped_files,
        )
        write_statecharts(io_string, binary_name, scope, indent=indent, is_example=True)
        write_repc_files(io_string, binary_name, scope, indent=indent)

        if condition != "ON":
            indent -= 1
        string = io_string.getvalue()
        if len(string) != 0:
            string = string.rstrip("\n")
            cm_fh.write(f"{condition_str}{string}\n")
            if condition != "ON":
                cm_fh.write(f"{spaces(indent)}endif()\n")

        handling_first_scope = False

    if not is_user_project:
        cm_fh.write(
            f"\ninstall(TARGETS {binary_name}\n"
            f'    RUNTIME DESTINATION "${{INSTALL_EXAMPLEDIR}}"\n'
            f'    BUNDLE DESTINATION "${{INSTALL_EXAMPLEDIR}}"\n'
            f'    LIBRARY DESTINATION "${{INSTALL_EXAMPLEDIR}}"\n'
            f")\n"
        )

    return binary_name


def write_plugin(cm_fh, scope, *, indent: int = 0) -> str:
    extra = []
    is_qml_plugin = any("qml_plugin" == s for s in scope.get("_LOADED"))
    qmake_target_name = scope.TARGET

    # Forward the original Qt5 plugin target name, to correctly name the
    # final library file name, and also for .prl generation.
    if qmake_target_name and not is_qml_plugin:
        extra.append(f"OUTPUT_NAME {qmake_target_name}")

    # In Qt 6 CMake, the CMake target name for a plugin should be the
    # same as it is in Qt5. qmake in Qt 5 derived the CMake target name
    # from the "plugin class name", so use that.
    # If the class name isn't empty, use that as the target name.
    # Otherwise use the of value qmake TARGET
    plugin_class_name = scope.get_string("PLUGIN_CLASS_NAME")
    if plugin_class_name:
        plugin_name = plugin_class_name
    else:
        plugin_name = qmake_target_name
    assert plugin_name

    # If the target name is derived from the class name, no need to
    # forward the class name.
    if plugin_class_name and plugin_class_name != plugin_name:
        extra.append(f"CLASS_NAME {plugin_class_name}")

    qmldir = None
    plugin_type = scope.get_string("PLUGIN_TYPE")
    plugin_function_name = get_cmake_api_call("qt_add_plugin")
    if plugin_type:
        extra.append(f"TYPE {plugin_type}")
    elif is_qml_plugin:
        plugin_function_name = get_cmake_api_call("qt_add_qml_module")
        qmldir = write_qml_plugin(cm_fh, plugin_name, scope, indent=indent, extra_lines=extra)
    else:
        target_path = scope.expandString("target.path")
        target_path = replace_path_constants(target_path, scope)
        if target_path:
            extra.append(f'INSTALL_DIRECTORY "{target_path}"')
        else:
            extra.append("SKIP_INSTALL")

    past_major_versions = scope.expandString("QML_PAST_MAJOR_VERSIONS")
    if past_major_versions:
        extra.append(f"PAST_MAJOR_VERSIONS {past_major_versions}")

    if "qmltypes" in scope.get("CONFIG"):
        extra.append("GENERATE_QMLTYPES")

    if "install_qmltypes" in scope.get("CONFIG"):
        extra.append("INSTALL_QMLTYPES")

    if "static" in scope.get("CONFIG"):
        extra.append("STATIC")

    plugin_extends = scope.get_string("PLUGIN_EXTENDS")
    if plugin_type != "platform" and plugin_extends == "-":
        extra.append("DEFAULT_IF FALSE")

    forward_target_info(scope, extra)

    write_main_part(
        cm_fh,
        plugin_name,
        "Plugin",
        plugin_function_name,
        scope,
        indent=indent,
        extra_lines=extra,
        known_libraries={},
        extra_keys=[],
    )

    if qmldir:
        write_qml_plugin_epilogue(cm_fh, plugin_name, scope, qmldir, indent)

    return plugin_name


def get_qml_import_version(scope: Scope, target: str) -> str:
    import_version = scope.get_string("IMPORT_VERSION")
    if not import_version:
        import_version = scope.get_string("QML_IMPORT_VERSION")
    if not import_version:
        import_major_version = scope.get_string("QML_IMPORT_MAJOR_VERSION")
        import_minor_version = scope.get_string("QML_IMPORT_MINOR_VERSION")

        if not import_major_version and not import_minor_version:
            raise RuntimeError(f"No QML_IMPORT_VERSION info found for target {target}.")

        if not import_minor_version:
            import_minor_version = str(0)
        import_version = f"{import_major_version}.{import_minor_version}"

    if import_version:
        replacements = [
            ("$$QT_MINOR_VERSION", "${PROJECT_VERSION_MINOR}"),
            ("$$QT_VERSION", "${PROJECT_VERSION}"),
        ]
        for needle, replacement in replacements:
            import_version = import_version.replace(needle, replacement)
    return import_version


def write_qml_module(
    cm_fh: IO[str],
    target: str,
    scope: Scope,
    scopes: List[Scope],
    resource: QtResource,
    extra_add_qml_module_args: List[str] = [],
    indent: int = 0,
):
    uri = scope.get_string("QML_IMPORT_NAME")
    if not uri:
        uri = target

    try:
        version = get_qml_import_version(scope, target)
    except RuntimeError:
        version = "${PROJECT_VERSION}"

    dest_dir = scope.expandString("DESTDIR")
    if dest_dir:
        dest_dir = f"${{CMAKE_CURRENT_BINARY_DIR}}/{dest_dir}"

    content = ""

    qml_dir = None
    qml_dir_dynamic_imports = False

    qmldir_file_path_list = scope.get_files("qmldir.files")
    assert len(qmldir_file_path_list) < 2, "File path must only contain one path"
    qmldir_file_path = qmldir_file_path_list[0] if qmldir_file_path_list else "qmldir"
    qmldir_file_path = os.path.join(os.getcwd(), qmldir_file_path[0])

    dynamic_qmldir = scope.get("DYNAMIC_QMLDIR")
    if os.path.exists(qmldir_file_path):
        qml_dir = QmlDir()
        qml_dir.from_file(qmldir_file_path)
    elif dynamic_qmldir:
        qml_dir = QmlDir()
        qml_dir.from_lines(dynamic_qmldir)
        qml_dir_dynamic_imports = True

        content += "set(module_dynamic_qml_imports\n    "
        if len(qml_dir.imports) != 0:
            content += "\n    ".join(qml_dir.imports)
        content += "\n)\n\n"

        for sc in scopes[1:]:
            import_list = []
            qml_imports = sc.get("DYNAMIC_QMLDIR")
            for qml_import in qml_imports:
                if not qml_import.startswith("import "):
                    raise RuntimeError(
                        "Only qmldir import statements expected in conditional scope!"
                    )
                import_list.append(qml_import[len("import ") :].replace(" ", "/"))
            if len(import_list) == 0:
                continue

            assert sc.condition

            content += f"if ({sc.condition})\n"
            content += "    list(APPEND module_dynamic_qml_imports\n        "
            content += "\n        ".join(import_list)
            content += "\n    )\nendif()\n\n"

    content += dedent(
        f"""\
        qt_add_qml_module({target}
            URI {uri}
            VERSION {version}
            """
    )

    if resource is not None:
        qml_files = list(filter(is_qtquick_source_file, resource.files.keys()))
        if qml_files:
            content += "    QML_FILES\n"
            for file in qml_files:
                content += f"        {file}\n"
        other_files = list(itertools.filterfalse(is_qtquick_source_file, resource.files.keys()))
        if other_files:
            content += "    RESOURCES\n"
            for file in other_files:
                content += f"        {file}\n"
        if resource.prefix != "/":
            content += f"    RESOURCE_PREFIX {resource.prefix}\n"
        if scope.TEMPLATE == "app":
            content += "    NO_RESOURCE_TARGET_PATH\n"
    if dest_dir:
        content += f"    OUTPUT_DIRECTORY {dest_dir}\n"

    if qml_dir is not None:
        if qml_dir.designer_supported:
            content += "    DESIGNER_SUPPORTED\n"
        if len(qml_dir.classname) != 0:
            content += f"    CLASSNAME {qml_dir.classname}\n"
        if len(qml_dir.depends) != 0:
            content += "    DEPENDENCIES\n"
            for dep in qml_dir.depends:
                content += f"        {dep[0]}/{dep[1]}\n"
        if len(qml_dir.type_names) == 0:
            content += "    SKIP_TYPE_REGISTRATION\n"
        if len(qml_dir.imports) != 0 and not qml_dir_dynamic_imports:
            qml_dir_imports_line = "        \n".join(qml_dir.imports)
            content += f"    IMPORTS\n{qml_dir_imports_line}"
        if qml_dir_dynamic_imports:
            content += "    IMPORTS ${module_dynamic_qml_imports}\n"
        if len(qml_dir.optional_imports) != 0:
            qml_dir_optional_imports_line = "        \n".join(qml_dir.optional_imports)
            content += f"    OPTIONAL_IMPORTS\n{qml_dir_optional_imports_line}"
        if qml_dir.plugin_optional:
            content += "    PLUGIN_OPTIONAL\n"

    for arg in extra_add_qml_module_args:
        content += "    "
        content += arg
        content += "\n"
    content += ")\n"

    if resource:
        content += write_resource_source_file_properties(
            sorted(resource.files.keys()),
            resource.files,
            resource.base_dir,
            resource.skip_qtquick_compiler,
        )

    content += "\n"
    cm_fh.write(content)


def write_qml_plugin(
    cm_fh: IO[str],
    target: str,
    scope: Scope,
    *,
    extra_lines: Optional[List[str]] = None,
    indent: int = 0,
    **kwargs: Any,
) -> Optional[QmlDir]:
    # Collect other args if available
    if extra_lines is None:
        extra_lines = []
    indent += 2

    target_path = scope.get_string("TARGETPATH")
    if target_path:
        uri = target_path.replace("/", ".")
        import_name = scope.get_string("IMPORT_NAME")
        # Catch special cases such as foo.QtQuick.2.bar, which when converted
        # into a target path via cmake will result in foo/QtQuick/2/bar, which is
        # not what we want. So we supply the target path override.
        target_path_from_uri = uri.replace(".", "/")
        if target_path != target_path_from_uri:
            extra_lines.append(f'TARGET_PATH "{target_path}"')
        if import_name:
            extra_lines.append(f'URI "{import_name}"')
        else:
            uri = re.sub("\\.\\d+", "", uri)
            extra_lines.append(f'URI "{uri}"')

    import_version = get_qml_import_version(scope, target)
    if import_version:
        extra_lines.append(f'VERSION "{import_version}"')

    plugindump_dep = scope.get_string("QML_PLUGINDUMP_DEPENDENCIES")

    if plugindump_dep:
        extra_lines.append(f'QML_PLUGINDUMP_DEPENDENCIES "{plugindump_dep}"')

    qml_dir = None
    qmldir_file_path = os.path.join(os.getcwd(), "qmldir")
    qml_dir_dynamic_imports = False
    if os.path.exists(qmldir_file_path):
        qml_dir = QmlDir()
        qml_dir.from_file(qmldir_file_path)
    else:
        dynamic_qmldir = scope.get("DYNAMIC_QMLDIR")
        if not dynamic_qmldir:
            return None
        qml_dir = QmlDir()
        qml_dir.from_lines(dynamic_qmldir)
        qml_dir_dynamic_imports = True

        # Check scopes for conditional entries
        scopes = flatten_scopes(scope)
        cm_fh.write("set(module_dynamic_qml_imports\n    ")
        if len(qml_dir.imports) != 0:
            cm_fh.write("\n    ".join(qml_dir.imports))
        cm_fh.write("\n)\n\n")

        for sc in scopes[1:]:
            import_list = []
            qml_imports = sc.get("DYNAMIC_QMLDIR")
            for qml_import in qml_imports:
                if not qml_import.startswith("import "):
                    raise RuntimeError(
                        "Only qmldir import statements expected in conditional scope!"
                    )
                import_list.append(qml_import[len("import ") :].replace(" ", "/"))
            if len(import_list) == 0:
                continue

            assert sc.condition

            cm_fh.write(f"if ({sc.condition})\n")
            cm_fh.write("    list(APPEND module_dynamic_qml_imports\n        ")
            cm_fh.write("\n        ".join(import_list))
            cm_fh.write("\n    )\nendif()\n\n")

    if qml_dir is not None:
        if qml_dir.designer_supported:
            extra_lines.append("DESIGNER_SUPPORTED")
        if len(qml_dir.classname) != 0:
            extra_lines.append(f"CLASSNAME {qml_dir.classname}")
        if len(qml_dir.depends) != 0:
            extra_lines.append("DEPENDENCIES")
            for dep in qml_dir.depends:
                extra_lines.append(f"    {dep[0]}/{dep[1]}")
        if len(qml_dir.type_names) == 0:
            extra_lines.append("SKIP_TYPE_REGISTRATION")
        if len(qml_dir.imports) != 0 and not qml_dir_dynamic_imports:
            qml_dir_imports_line = "\n        ".join(qml_dir.imports)
            extra_lines.append("IMPORTS\n        " f"{qml_dir_imports_line}")
        if qml_dir_dynamic_imports:
            extra_lines.append("IMPORTS ${module_dynamic_qml_imports}")
        if len(qml_dir.optional_imports):
            qml_dir_optional_imports_line = "\n        ".join(qml_dir.optional_imports)
            extra_lines.append("OPTIONAL_IMPORTS\n        " f"{qml_dir_optional_imports_line}")
        if qml_dir.plugin_optional:
            extra_lines.append("PLUGIN_OPTIONAL")

    return qml_dir


def write_qml_plugin_epilogue(
    cm_fh: IO[str], target: str, scope: Scope, qmldir: QmlDir, indent: int = 0
):

    qml_files = scope.get_files("QML_FILES", use_vpath=True)
    if qml_files:

        indent_0 = spaces(indent)
        indent_1 = spaces(indent + 1)
        # Quote file paths in case there are spaces.
        qml_files_quoted = [f'"{qf}"' for qf in qml_files]

        indented_qml_files = f"\n{indent_1}".join(qml_files_quoted)
        cm_fh.write(f"\n{indent_0}set(qml_files\n{indent_1}" f"{indented_qml_files}\n)\n")

        for qml_file in qml_files:
            if qml_file in qmldir.type_names:
                qmldir_file_info = qmldir.type_names[qml_file]
                cm_fh.write(f"{indent_0}set_source_files_properties({qml_file} PROPERTIES\n")
                cm_fh.write(f'{indent_1}QT_QML_SOURCE_VERSION "{qmldir_file_info.versions}"\n')
                # Only write typename if they are different, CMake will infer
                # the name by default
                if (
                    os.path.splitext(os.path.basename(qmldir_file_info.path))[0]
                    != qmldir_file_info.type_name
                ):
                    cm_fh.write(f"{indent_1}QT_QML_SOURCE_TYPENAME {qmldir_file_info.type_name}\n")
                if qmldir_file_info.singleton:
                    cm_fh.write(f"{indent_1}QT_QML_SINGLETON_TYPE TRUE\n")
                if qmldir_file_info.internal:
                    cm_fh.write(f"{indent_1}QT_QML_INTERNAL_TYPE TRUE\n")
                cm_fh.write(f"{indent_0})\n")
            else:
                cm_fh.write(
                    f"{indent_0}set_source_files_properties({qml_file} PROPERTIES\n"
                    f"{indent_1}QT_QML_SKIP_QMLDIR_ENTRY TRUE\n"
                    f"{indent_0})\n"
                )

        cm_fh.write(
            f"\n{indent_0}qt6_target_qml_files({target}\n{indent_1}FILES\n"
            f"{spaces(indent+2)}${{qml_files}}\n)\n"
        )


def handle_app_or_lib(
    scope: Scope,
    cm_fh: IO[str],
    *,
    indent: int = 0,
    is_example: bool = False,
    is_user_project=False,
) -> None:
    assert scope.TEMPLATE in ("app", "lib")

    config = scope.get("CONFIG")
    is_jar = "java" in config
    is_lib = scope.TEMPLATE == "lib"
    is_qml_plugin = any("qml_plugin" == s for s in scope.get("_LOADED"))
    is_plugin = "plugin" in config
    is_qt_plugin = any("qt_plugin" == s for s in scope.get("_LOADED")) or is_qml_plugin
    target = ""
    target_ref = None
    gui = all(val not in config for val in ["console", "cmdline", "-app_bundle"]) and all(
        val not in scope.expand("QT") for val in ["testlib", "testlib-private"]
    )

    if is_jar:
        write_jar(cm_fh, scope, indent=indent)
    elif "qt_helper_lib" in scope.get("_LOADED"):
        assert not is_example
        target = write_3rdparty_library(cm_fh, scope, indent=indent)
    elif is_example:
        target = write_example(
            cm_fh, scope, gui, indent=indent, is_plugin=is_plugin, is_user_project=is_user_project
        )
    elif is_qt_plugin:
        assert not is_example
        target = write_plugin(cm_fh, scope, indent=indent)
    elif (is_lib and "qt_module" not in scope.get("_LOADED")) or is_plugin:
        assert not is_example
        target = write_generic_library(cm_fh, scope, indent=indent)
    elif is_lib or "qt_module" in scope.get("_LOADED"):
        assert not is_example
        target = write_module(cm_fh, scope, indent=indent)
    elif "qt_tool" in scope.get("_LOADED"):
        assert not is_example
        target, target_ref = write_tool(cm_fh, scope, indent=indent)
    elif "qt_app" in scope.get("_LOADED"):
        assert not is_example
        scope._is_internal_qt_app = True
        target = write_qt_app(cm_fh, scope, indent=indent)
    else:
        if "testcase" in config or "testlib" in config or "qmltestcase" in config:
            assert not is_example
            target = write_test(cm_fh, scope, gui, indent=indent)
        else:
            target = write_binary(cm_fh, scope, gui, indent=indent)

    if target_ref is None:
        target_ref = target

    # ind = spaces(indent)
    cmake_api_call = get_cmake_api_call("qt_add_docs")
    write_source_file_list(
        cm_fh,
        scope,
        "",
        ["QMAKE_DOCS"],
        indent,
        header=f"{cmake_api_call}({target_ref}\n",
        footer=")\n",
    )

    # Generate qmltypes instruction for anything that may have CONFIG += qmltypes
    # that is not a qml plugin
    if (
        not is_example
        and "qmltypes" in scope.get("CONFIG")
        and "qml_plugin" not in scope.get("_LOADED")
    ):
        cm_fh.write(f"\n{spaces(indent)}set_target_properties({target_ref} PROPERTIES\n")

        install_dir = scope.expandString("QMLTYPES_INSTALL_DIR")
        if install_dir:
            cm_fh.write(f"{spaces(indent+1)}QT_QML_MODULE_INSTALL_QMLTYPES TRUE\n")

        import_version = get_qml_import_version(scope, target)
        if import_version:
            cm_fh.write(f"{spaces(indent+1)}QT_QML_MODULE_VERSION {import_version}\n")

        past_major_versions = scope.expandString("QML_PAST_MAJOR_VERSIONS")
        if past_major_versions:
            cm_fh.write(f"{spaces(indent+1)}QT_QML_PAST_MAJOR_VERSIONS {past_major_versions}\n")

        import_name = scope.expandString("QML_IMPORT_NAME")
        if import_name:
            cm_fh.write(f"{spaces(indent+1)}QT_QML_MODULE_URI {import_name}\n")

        json_output_filename = scope.expandString("QMLTYPES_FILENAME")
        if json_output_filename:
            cm_fh.write(f"{spaces(indent+1)}QT_QMLTYPES_FILENAME {json_output_filename}\n")

        target_path = scope.get("TARGETPATH")
        if target_path:
            cm_fh.write(f"{spaces(indent+1)}QT_QML_MODULE_TARGET_PATH {target_path}\n")

        if install_dir:
            install_dir = install_dir.replace("$$[QT_INSTALL_QML]", "${INSTALL_QMLDIR}")
            cm_fh.write(f'{spaces(indent+1)}QT_QML_MODULE_INSTALL_DIR "{install_dir}"\n')

        cm_fh.write(f"{spaces(indent)})\n\n")
        cm_fh.write(f"qt6_qml_type_registration({target_ref})\n")


def handle_top_level_repo_project(scope: Scope, cm_fh: IO[str]):
    # qtdeclarative
    project_file_name = os.path.splitext(os.path.basename(scope.file_absolute_path))[0]

    # declarative
    file_name_without_qt_prefix = project_file_name[2:]

    # Qt::Declarative
    qt_lib = map_qt_library(file_name_without_qt_prefix)

    # Found a mapping, adjust name.
    if qt_lib != file_name_without_qt_prefix:
        # QtDeclarative
        qt_lib = re.sub(r":", r"", qt_lib)

        # Declarative
        qt_lib_no_prefix = qt_lib[2:]
    else:
        qt_lib += "_FIXME"
        qt_lib_no_prefix = qt_lib

    header = dedent(
        f"""\
                cmake_minimum_required(VERSION {cmake_version_string})

                include(.cmake.conf)
                project({qt_lib}
                    VERSION "${{QT_REPO_MODULE_VERSION}}"
                    DESCRIPTION "Qt {qt_lib_no_prefix} Libraries"
                    HOMEPAGE_URL "https://qt.io/"
                    LANGUAGES CXX C
                )

                find_package(Qt6 ${{PROJECT_VERSION}} CONFIG REQUIRED COMPONENTS BuildInternals Core SET_ME_TO_SOMETHING_USEFUL)
                find_package(Qt6 ${{PROJECT_VERSION}} CONFIG OPTIONAL_COMPONENTS SET_ME_TO_SOMETHING_USEFUL)

                """
    )

    build_repo = dedent(
        """\
                qt_build_repo()
                """
    )

    cm_fh.write(f"{header}{expand_project_requirements(scope)}{build_repo}")


def create_top_level_cmake_conf():
    conf_file_name = ".cmake.conf"
    try:
        with open(conf_file_name, "x") as file:
            file.write('set(QT_REPO_MODULE_VERSION "6.6.1")\n')
    except FileExistsError:
        pass


def find_top_level_repo_project_file(project_file_path: str = "") -> Optional[str]:
    qmake_or_cmake_conf_path = find_qmake_or_cmake_conf(project_file_path)
    qmake_or_cmake_dir = os.path.dirname(qmake_or_cmake_conf_path)

    # Hope to a programming god that there's only one .pro file at the
    # top level directory of repository.
    glob_result = glob.glob(os.path.join(qmake_or_cmake_dir, "*.pro"))
    if len(glob_result) > 0:
        return glob_result[0]
    return None


def handle_top_level_repo_tests_project(scope: Scope, cm_fh: IO[str]):

    content = dedent(
        """\
        if(QT_BUILD_STANDALONE_TESTS)
            # Add qt_find_package calls for extra dependencies that need to be found when building
            # the standalone tests here.
        endif()
        qt_build_tests()
"""
    )

    cm_fh.write(f"{content}")


def write_regular_cmake_target_scope_section(
    scope: Scope, cm_fh: IO[str], indent: int = 0, skip_sources: bool = False
):
    if not skip_sources:
        target_sources = "target_sources(${PROJECT_NAME} PUBLIC"
        write_all_source_file_lists(cm_fh, scope, target_sources, indent=indent, footer=")")

    write_include_paths(
        cm_fh,
        scope,
        "target_include_directories(${{PROJECT_NAME}} PUBLIC",
        indent=indent,
        footer=")",
    )
    write_defines(
        cm_fh,
        scope,
        "target_compile_definitions(${{PROJECT_NAME}} PUBLIC",
        indent=indent,
        footer=")",
    )
    (public_libs, private_libs) = extract_cmake_libraries(scope)
    write_list(
        cm_fh,
        private_libs,
        "",
        indent=indent,
        header="target_link_libraries(${{PROJECT_NAME}} PRIVATE\n",
        footer=")",
    )
    write_list(
        cm_fh,
        public_libs,
        "",
        indent=indent,
        header="target_link_libraries(${{PROJECT_NAME}} PUBLIC\n",
        footer=")",
    )
    write_compile_options(
        cm_fh, scope, "target_compile_options(${{PROJECT_NAME}}", indent=indent, footer=")"
    )


def handle_config_test_project(scope: Scope, cm_fh: IO[str]):
    project_name = os.path.splitext(os.path.basename(scope.file_absolute_path))[0]
    content = (
        f"cmake_minimum_required(VERSION 3.16)\n"
        f"project(config_test_{project_name} LANGUAGES C CXX)\n"
        """
if(DEFINED QT_CONFIG_COMPILE_TEST_CMAKE_SYSTEM_PREFIX_PATH)
    set(CMAKE_SYSTEM_PREFIX_PATH "${QT_CONFIG_COMPILE_TEST_CMAKE_SYSTEM_PREFIX_PATH}")
endif()
if(DEFINED QT_CONFIG_COMPILE_TEST_CMAKE_SYSTEM_FRAMEWORK_PATH)
    set(CMAKE_SYSTEM_FRAMEWORK_PATH "${QT_CONFIG_COMPILE_TEST_CMAKE_SYSTEM_FRAMEWORK_PATH}")
endif()

foreach(p ${QT_CONFIG_COMPILE_TEST_PACKAGES})
    find_package(${p})
endforeach()

if(QT_CONFIG_COMPILE_TEST_LIBRARIES)
    link_libraries(${QT_CONFIG_COMPILE_TEST_LIBRARIES})
endif()
if(QT_CONFIG_COMPILE_TEST_LIBRARY_TARGETS)
    foreach(lib ${QT_CONFIG_COMPILE_TEST_LIBRARY_TARGETS})
        if(TARGET ${lib})
            link_libraries(${lib})
        endif()
    endforeach()
endif()
"""
    )
    cm_fh.write(f"{content}\n")

    # Remove default QT libs.
    scope._append_operation("QT", RemoveOperation(["core", "gui"]))

    add_target = "add_executable(${{PROJECT_NAME}}"

    temp_buffer = io.StringIO()
    write_all_source_file_lists(temp_buffer, scope, add_target, indent=0)
    buffer_value = temp_buffer.getvalue()

    if buffer_value:
        cm_fh.write(buffer_value)
    else:
        cm_fh.write(add_target)
    cm_fh.write(")\n")

    indent = 0
    write_regular_cmake_target_scope_section(scope, cm_fh, indent, skip_sources=True)

    recursive_evaluate_scope(scope)
    scopes = flatten_scopes(scope)
    scopes = merge_scopes(scopes)

    assert len(scopes)
    assert scopes[0].total_condition == "ON"

    for c in scopes[1:]:
        extend_scope_io_string = io.StringIO()
        write_regular_cmake_target_scope_section(c, extend_scope_io_string, indent=indent + 1)
        extend_string = extend_scope_io_string.getvalue()

        if extend_string:
            assert c.total_condition, "Cannot write if with empty condition"
            extend_scope = (
                f"\nif({map_to_cmake_condition(c.total_condition)})\n"
                f"{extend_string}"
                f"endif()\n"
            )
            cm_fh.write(extend_scope)


def cmakeify_scope(
    scope: Scope,
    cm_fh: IO[str],
    *,
    indent: int = 0,
    is_example: bool = False,
    is_user_project: bool = False,
) -> None:
    template = scope.TEMPLATE

    if is_user_project:
        if template == "subdirs":
            handle_subdir(scope, cm_fh, indent=indent, is_example=True, is_user_project=True)
        elif template in ("app", "lib"):
            handle_app_or_lib(scope, cm_fh, indent=indent, is_example=True, is_user_project=True)
    else:
        temp_buffer = io.StringIO()

        # Handle top level repo project in a special way.
        if is_top_level_repo_project(scope.file_absolute_path):
            create_top_level_cmake_conf()
            handle_top_level_repo_project(scope, temp_buffer)
        # Same for top-level tests.
        elif is_top_level_repo_tests_project(scope.file_absolute_path):
            handle_top_level_repo_tests_project(scope, temp_buffer)
        elif is_config_test_project(scope.file_absolute_path):
            handle_config_test_project(scope, temp_buffer)
        elif template == "subdirs":
            handle_subdir(scope, temp_buffer, indent=indent, is_example=is_example)
        elif template in ("app", "lib"):
            handle_app_or_lib(scope, temp_buffer, indent=indent, is_example=is_example)
        else:
            print(f"    XXXX: {scope.file}: Template type {template} not yet supported.")

        buffer_value = temp_buffer.getvalue()

        if is_top_level_repo_examples_project(scope.file_absolute_path):
            # Wrap top level examples project with some commands which
            # are necessary to build examples as part of the overall
            # build.
            buffer_value = f"qt_examples_build_begin()\n\n{buffer_value}\nqt_examples_build_end()\n"

        cm_fh.write(buffer_value)


def generate_new_cmakelists(
    scope: Scope, *, is_example: bool = False, is_user_project: bool = True, debug: bool = False
) -> None:
    if debug:
        print("Generating CMakeLists.gen.txt")
    with open(scope.generated_cmake_lists_path, "w") as cm_fh:
        assert scope.file
        cm_fh.write(f"# Generated from {os.path.basename(scope.file)}.\n\n")

        is_example_heuristic = is_example_project(scope.file_absolute_path)
        final_is_example_decision = is_example or is_example_heuristic
        cmakeify_scope(
            scope, cm_fh, is_example=final_is_example_decision, is_user_project=is_user_project
        )


def do_include(scope: Scope, *, debug: bool = False) -> None:
    for c in scope.children:
        do_include(c)

    for include_index, include_file in enumerate(scope.get_files("_INCLUDED", is_include=True)):
        if not include_file:
            continue
        # Ignore selfcover.pri as this generates too many incompatible flags
        # need to be removed with special cases
        if include_file.endswith("selfcover.pri"):
            continue
        if include_file.startswith("${QT_SOURCE_TREE}"):
            root_source_dir = get_top_level_repo_project_path(scope.file_absolute_path)
            include_file = include_file.replace("${QT_SOURCE_TREE}", root_source_dir)
        if not os.path.isfile(include_file):
            generated_config_pri_pattern = re.compile(r"qt.+?-config\.pri$")
            match_result = re.search(generated_config_pri_pattern, include_file)
            if not match_result:
                print(f"    XXXX: Failed to include {include_file}.")
            continue

        include_op = scope._get_operation_at_index("_INCLUDED", include_index)
        include_line_no = include_op._line_no

        include_result, project_file_content = parseProFile(include_file, debug=debug)
        include_scope = Scope.FromDict(
            None,
            include_file,
            include_result.asDict().get("statements"),
            "",
            scope.basedir,
            project_file_content=project_file_content,
            parent_include_line_no=include_line_no,
        )  # This scope will be merged into scope!

        do_include(include_scope)

        scope.merge(include_scope)


def copy_generated_file_to_final_location(
    scope: Scope, output_file: str, keep_temporary_files=False, debug: bool = False
) -> None:
    if debug:
        print(f"Copying {scope.generated_cmake_lists_path} to {output_file}")

    base_dir = os.path.dirname(output_file)
    base_dir_abs = os.path.realpath(base_dir)
    os.makedirs(base_dir_abs, exist_ok=True)

    copyfile(scope.generated_cmake_lists_path, output_file)
    if not keep_temporary_files:
        os.remove(scope.generated_cmake_lists_path)


def cmake_project_has_skip_marker(project_file_path: str = "") -> bool:
    dir_path = os.path.dirname(project_file_path)
    cmake_project_path = os.path.join(dir_path, "CMakeLists.txt")
    if not os.path.exists(cmake_project_path):
        return False

    with open(cmake_project_path, "r") as file_fd:
        contents = file_fd.read()

    if "# special case skip regeneration" in contents:
        return True

    return False


def should_convert_project(project_file_path: str = "", ignore_skip_marker: bool = False) -> bool:
    qmake_or_cmake_conf_path = find_qmake_or_cmake_conf(project_file_path)
    qmake_or_cmake_conf_dir_path = os.path.dirname(qmake_or_cmake_conf_path)

    project_relative_path = os.path.relpath(project_file_path, qmake_or_cmake_conf_dir_path)

    # Skip cmake auto tests, they should not be converted.
    if project_relative_path.startswith("tests/auto/cmake"):
        return False
    if project_relative_path.startswith("tests/auto/installed_cmake"):
        return False

    # Skip qmake testdata projects.
    if project_relative_path.startswith("tests/auto/tools/qmake/testdata"):
        return False

    # Skip doc snippets.
    if fnmatch.fnmatch(project_relative_path, "src/*/doc/snippets/*"):
        return False

    # Skip certain config tests.
    config_tests = [
        # Relative to qtbase/config.tests
        "arch/arch.pro",
        "avx512/avx512.pro",
        "stl/stl.pro",
        "verifyspec/verifyspec.pro",
        "x86_simd/x86_simd.pro",
        # Relative to repo src dir
        "config.tests/hostcompiler/hostcompiler.pro",
    ]
    skip_certain_tests = any(project_relative_path.startswith(c) for c in config_tests)
    if skip_certain_tests:
        return False

    # Skip if CMakeLists.txt in the same path as project_file_path has a
    # special skip marker.
    if not ignore_skip_marker and cmake_project_has_skip_marker(project_file_path):
        return False

    return True


def should_convert_project_after_parsing(
    file_scope: Scope, skip_subdirs_project: bool = False
) -> bool:
    template = file_scope.TEMPLATE
    if template == "subdirs" and skip_subdirs_project:
        return False
    return True


def main() -> None:
    # Be sure of proper Python version
    assert sys.version_info >= (3, 7)

    args = _parse_commandline()

    debug_parsing = args.debug_parser or args.debug
    if args.skip_condition_cache:
        set_condition_simplified_cache_enabled(False)

    backup_current_dir = os.getcwd()

    for file in args.files:
        new_current_dir = os.path.dirname(file)
        file_relative_path = os.path.basename(file)
        if new_current_dir:
            os.chdir(new_current_dir)

        project_file_absolute_path = os.path.abspath(file_relative_path)
        if not should_convert_project(project_file_absolute_path, args.ignore_skip_marker):
            print(f'Skipping conversion of project: "{project_file_absolute_path}"')
            continue

        parseresult, project_file_content = parseProFile(file_relative_path, debug=debug_parsing)

        # If CMake api version is given on command line, that means the
        # user wants to force use that api version.
        global cmake_api_version
        if args.api_version:
            cmake_api_version = args.api_version
        else:
            # Otherwise detect the api version in the old CMakeLists.txt
            # if it exists.
            detected_cmake_api_version = detect_cmake_api_version_used_in_file_content(
                file_relative_path
            )
            if detected_cmake_api_version:
                cmake_api_version = detected_cmake_api_version

        if args.debug_parse_result or args.debug:
            print("\n\n#### Parser result:")
            print(parseresult)
            print("\n#### End of parser result.\n")
        if args.debug_parse_dictionary or args.debug:
            print("\n\n####Parser result dictionary:")
            print(parseresult.asDict())
            print("\n#### End of parser result dictionary.\n")

        file_scope = Scope.FromDict(
            None,
            file_relative_path,
            parseresult.asDict().get("statements"),
            project_file_content=project_file_content,
        )

        if args.debug_pro_structure or args.debug:
            print("\n\n#### .pro/.pri file structure:")
            file_scope.dump()
            print("\n#### End of .pro/.pri file structure.\n")

        do_include(file_scope, debug=debug_parsing)

        if args.debug_full_pro_structure or args.debug:
            print("\n\n#### Full .pro/.pri file structure:")
            file_scope.dump()
            print("\n#### End of full .pro/.pri file structure.\n")

        if not should_convert_project_after_parsing(file_scope, args.skip_subdirs_project):
            print(f'Skipping conversion of project: "{project_file_absolute_path}"')
            continue

        generate_new_cmakelists(
            file_scope,
            is_example=args.is_example,
            is_user_project=args.is_user_project,
            debug=args.debug,
        )

        copy_generated_file = True

        output_file = file_scope.original_cmake_lists_path
        if args.output_file:
            output_file = args.output_file

        if not args.skip_special_case_preservation:
            debug_special_case = args.debug_special_case_preservation or args.debug
            handler = SpecialCaseHandler(
                output_file,
                file_scope.generated_cmake_lists_path,
                file_scope.basedir,
                keep_temporary_files=args.keep_temporary_files,
                debug=debug_special_case,
            )

            copy_generated_file = handler.handle_special_cases()

        if copy_generated_file:
            copy_generated_file_to_final_location(
                file_scope, output_file, keep_temporary_files=args.keep_temporary_files
            )
        os.chdir(backup_current_dir)


if __name__ == "__main__":
    main()
