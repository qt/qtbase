#!/usr/bin/env python3
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import json_parser
import posixpath
import re
import sys
from typing import Optional, Set
from textwrap import dedent
import os

from special_case_helper import SpecialCaseHandler
from helper import (
    map_qt_library,
    featureName,
    map_platform,
    find_3rd_party_library_mapping,
    generate_find_package_info,
    get_compile_test_dependent_library_mapping,
)

knownTests = set()  # type: Set[str]


class LibraryMapping:
    def __init__(self, package: str, resultVariable: str, appendFoundSuffix: bool = True) -> None:
        self.package = package
        self.resultVariable = resultVariable
        self.appendFoundSuffix = appendFoundSuffix


def map_tests(test: str) -> Optional[str]:
    testmap = {
        "c99": "c_std_99 IN_LIST CMAKE_C_COMPILE_FEATURES",
        "c11": "c_std_11 IN_LIST CMAKE_C_COMPILE_FEATURES",
        "x86SimdAlways": "ON",  # FIXME: Make this actually do a compile test.
        "aesni": "TEST_subarch_aesni",
        "avx": "TEST_subarch_avx",
        "avx2": "TEST_subarch_avx2",
        "avx512f": "TEST_subarch_avx512f",
        "avx512cd": "TEST_subarch_avx512cd",
        "avx512dq": "TEST_subarch_avx512dq",
        "avx512bw": "TEST_subarch_avx512bw",
        "avx512er": "TEST_subarch_avx512er",
        "avx512pf": "TEST_subarch_avx512pf",
        "avx512vl": "TEST_subarch_avx512vl",
        "avx512ifma": "TEST_subarch_avx512ifma",
        "avx512vbmi": "TEST_subarch_avx512vbmi",
        "avx512vbmi2": "TEST_subarch_avx512vbmi2",
        "avx512vpopcntdq": "TEST_subarch_avx512vpopcntdq",
        "avx5124fmaps": "TEST_subarch_avx5124fmaps",
        "avx5124vnniw": "TEST_subarch_avx5124vnniw",
        "bmi": "TEST_subarch_bmi",
        "bmi2": "TEST_subarch_bmi2",
        "cx16": "TEST_subarch_cx16",
        "f16c": "TEST_subarch_f16c",
        "fma": "TEST_subarch_fma",
        "fma4": "TEST_subarch_fma4",
        "fsgsbase": "TEST_subarch_fsgsbase",
        "gfni": "TEST_subarch_gfni",
        "ibt": "TEST_subarch_ibt",
        "libclang": "TEST_libclang",
        "lwp": "TEST_subarch_lwp",
        "lzcnt": "TEST_subarch_lzcnt",
        "mmx": "TEST_subarch_mmx",
        "movbe": "TEST_subarch_movbe",
        "mpx": "TEST_subarch_mpx",
        "no-sahf": "TEST_subarch_no_shaf",
        "pclmul": "TEST_subarch_pclmul",
        "popcnt": "TEST_subarch_popcnt",
        "prefetchwt1": "TEST_subarch_prefetchwt1",
        "prfchw": "TEST_subarch_prfchw",
        "pdpid": "TEST_subarch_rdpid",
        "rdpid": "TEST_subarch_rdpid",
        "rdseed": "TEST_subarch_rdseed",
        "rdrnd": "TEST_subarch_rdrnd",
        "rtm": "TEST_subarch_rtm",
        "shani": "TEST_subarch_shani",
        "shstk": "TEST_subarch_shstk",
        "sse2": "TEST_subarch_sse2",
        "sse3": "TEST_subarch_sse3",
        "ssse3": "TEST_subarch_ssse3",
        "sse4a": "TEST_subarch_sse4a",
        "sse4_1": "TEST_subarch_sse4_1",
        "sse4_2": "TEST_subarch_sse4_2",
        "tbm": "TEST_subarch_tbm",
        "xop": "TEST_subarch_xop",
        "neon": "TEST_subarch_neon",
        "iwmmxt": "TEST_subarch_iwmmxt",
        "crc32": "TEST_subarch_crc32",
        "vis": "TEST_subarch_vis",
        "vis2": "TEST_subarch_vis2",
        "vis3": "TEST_subarch_vis3",
        "dsp": "TEST_subarch_dsp",
        "dspr2": "TEST_subarch_dspr2",
        "altivec": "TEST_subarch_altivec",
        "spe": "TEST_subarch_spe",
        "vsx": "TEST_subarch_vsx",
        "openssl11": '(OPENSSL_VERSION VERSION_GREATER_EQUAL "1.1.0")',
        "libinput_axis_api": "ON",
        "xlib": "X11_FOUND",
        "wayland-scanner": "WaylandScanner_FOUND",
        "3rdparty-hunspell": "VKB_HAVE_3RDPARTY_HUNSPELL",
        "t9write-alphabetic": "VKB_HAVE_T9WRITE_ALPHA",
        "t9write-cjk": "VKB_HAVE_T9WRITE_CJK",
    }
    if test in testmap:
        return testmap.get(test, None)
    if test in knownTests:
        return f"TEST_{featureName(test)}"
    return None


def cm(ctx, *output):
    txt = ctx["output"]
    if txt != "" and not txt.endswith("\n"):
        txt += "\n"
    txt += "\n".join(output)

    ctx["output"] = txt
    return ctx


def readJsonFromDir(path: str) -> str:
    path = posixpath.join(path, "configure.json")

    print(f"Reading {path}...")
    assert posixpath.exists(path)

    parser = json_parser.QMakeSpecificJSONParser()
    return parser.parse(path)


def processFiles(ctx, data):
    print("  files:")
    if "files" in data:
        for (k, v) in data["files"].items():
            ctx[k] = v
    return ctx


def parseLib(ctx, lib, data, cm_fh, cmake_find_packages_set):
    newlib = find_3rd_party_library_mapping(lib)
    if not newlib:
        print(f'    XXXX Unknown library "{lib}".')
        return

    if newlib.packageName is None:
        print(f'    **** Skipping library "{lib}" -- was masked.')
        return

    print(f"    mapped library {lib} to {newlib.targetName}.")

    # Avoid duplicate find_package calls.
    if newlib.targetName in cmake_find_packages_set:
        return

    # If certain libraries are used within a feature, but the feature
    # is only emitted conditionally with a simple condition (like
    # 'on Windows' or 'on Linux'), we should enclose the find_package
    # call for the library into the same condition.
    emit_if = newlib.emit_if

    # Only look through features if a custom emit_if wasn't provided.
    if not emit_if:
        for feature in data["features"]:
            feature_data = data["features"][feature]
            if (
                "condition" in feature_data
                and f"libs.{lib}" in feature_data["condition"]
                and "emitIf" in feature_data
                and "config." in feature_data["emitIf"]
            ):
                emit_if = feature_data["emitIf"]
                break

    if emit_if:
        emit_if = map_condition(emit_if)

    cmake_find_packages_set.add(newlib.targetName)

    find_package_kwargs = {"emit_if": emit_if}
    if newlib.is_bundled_with_qt:
        # If a library is bundled with Qt, it has 2 FindFoo.cmake
        # modules: WrapFoo and WrapSystemFoo.
        # FindWrapSystemFoo.cmake will try to find the 'Foo' library in
        # the usual CMake locations, and will create a
        # WrapSystemFoo::WrapSystemFoo target pointing to the library.
        #
        # FindWrapFoo.cmake will create a WrapFoo::WrapFoo target which
        # will link either against the WrapSystemFoo or QtBundledFoo
        # target depending on certain feature values.
        #
        # Because the following qt_find_package call is for
        # configure.cmake consumption, we make the assumption that
        # configure.cmake is interested in finding the system library
        # for the purpose of enabling or disabling a system_foo feature.
        find_package_kwargs["use_system_package_name"] = True
    find_package_kwargs["module"] = ctx["module"]

    cm_fh.write(generate_find_package_info(newlib, **find_package_kwargs))

    if "use" in data["libraries"][lib]:
        use_entry = data["libraries"][lib]["use"]
        if isinstance(use_entry, str):
            print(f"1use: {use_entry}")
            cm_fh.write(f"qt_add_qmake_lib_dependency({newlib.soName} {use_entry})\n")
        else:
            for use in use_entry:
                print(f"2use: {use}")
                indentation = ""
                has_condition = False
                if "condition" in use:
                    has_condition = True
                    indentation = "    "
                    condition = map_condition(use["condition"])
                    cm_fh.write(f"if({condition})\n")
                cm_fh.write(
                    f"{indentation}qt_add_qmake_lib_dependency({newlib.soName} {use['lib']})\n"
                )
                if has_condition:
                    cm_fh.write("endif()\n")

    run_library_test = False
    mapped_library = find_3rd_party_library_mapping(lib)
    if mapped_library:
        run_library_test = mapped_library.run_library_test

    if run_library_test and "test" in data["libraries"][lib]:
        test = data["libraries"][lib]["test"]
        write_compile_test(
            ctx, lib, test, data, cm_fh, manual_library_list=[lib], is_library_test=True
        )


def lineify(label, value, quote=True):
    if value:
        if quote:
            escaped_value = value.replace('"', '\\"')
            return f'    {label} "{escaped_value}"\n'
        return f"    {label} {value}\n"
    return ""


def map_condition(condition):
    # Handle NOT:
    if isinstance(condition, list):
        condition = "(" + ") AND (".join(condition) + ")"
    if isinstance(condition, bool):
        if condition:
            return "ON"
        else:
            return "OFF"
    assert isinstance(condition, str)

    mapped_features = {"gbm": "gbm_FOUND"}

    # Turn foo != "bar" into (NOT foo STREQUAL 'bar')
    condition = re.sub(r"([^ ]+)\s*!=\s*('.*?')", "(! \\1 == \\2)", condition)
    # Turn foo != 156 into (NOT foo EQUAL 156)
    condition = re.sub(r"([^ ]+)\s*!=\s*([0-9]?)", "(! \\1 EQUAL \\2)", condition)

    condition = condition.replace("!", "NOT ")
    condition = condition.replace("&&", " AND ")
    condition = condition.replace("||", " OR ")
    condition = condition.replace("==", " STREQUAL ")

    # explicitly handle input.sdk == '':
    condition = re.sub(r"input\.sdk\s*==\s*''", "NOT INPUT_SDK", condition)

    last_pos = 0
    mapped_condition = ""
    has_failed = False
    for match in re.finditer(r"([a-zA-Z0-9_]+)\.([a-zA-Z0-9_+-]+)", condition):
        substitution = None
        # appendFoundSuffix = True
        if match.group(1) == "libs":
            libmapping = find_3rd_party_library_mapping(match.group(2))

            if libmapping and libmapping.packageName:
                substitution = libmapping.packageName
                if libmapping.resultVariable:
                    substitution = libmapping.resultVariable
                if libmapping.appendFoundSuffix:
                    substitution += "_FOUND"

                # Assume that feature conditions are interested whether
                # a system library is found, rather than the bundled one
                # which we always know we can build.
                if libmapping.is_bundled_with_qt:
                    substitution = substitution.replace("Wrap", "WrapSystem")

        elif match.group(1) == "features":
            feature = match.group(2)
            if feature in mapped_features:
                substitution = mapped_features.get(feature)
            else:
                substitution = f"QT_FEATURE_{featureName(match.group(2))}"

        elif match.group(1) == "subarch":
            substitution = f"TEST_arch_{'${TEST_architecture_arch}'}_subarch_{match.group(2)}"

        elif match.group(1) == "call":
            if match.group(2) == "crossCompile":
                substitution = "CMAKE_CROSSCOMPILING"

        elif match.group(1) == "tests":
            substitution = map_tests(match.group(2))

        elif match.group(1) == "input":
            substitution = f"INPUT_{featureName(match.group(2))}"

        elif match.group(1) == "config":
            substitution = map_platform(match.group(2))
        elif match.group(1) == "module":
            substitution = f"TARGET {map_qt_library(match.group(2))}"

        elif match.group(1) == "arch":
            if match.group(2) == "i386":
                # FIXME: Does this make sense?
                substitution = "(TEST_architecture_arch STREQUAL i386)"
            elif match.group(2) == "x86_64":
                substitution = "(TEST_architecture_arch STREQUAL x86_64)"
            elif match.group(2) == "arm":
                # FIXME: Does this make sense?
                substitution = "(TEST_architecture_arch STREQUAL arm)"
            elif match.group(2) == "arm64":
                # FIXME: Does this make sense?
                substitution = "(TEST_architecture_arch STREQUAL arm64)"
            elif match.group(2) == "mips":
                # FIXME: Does this make sense?
                substitution = "(TEST_architecture_arch STREQUAL mips)"

        if substitution is None:
            print(f'    XXXX Unknown condition "{match.group(0)}"')
            has_failed = True
        else:
            mapped_condition += condition[last_pos : match.start(1)] + substitution
            last_pos = match.end(2)

    mapped_condition += condition[last_pos:]

    # Space out '(' and ')':
    mapped_condition = mapped_condition.replace("(", " ( ")
    mapped_condition = mapped_condition.replace(")", " ) ")

    # Prettify:
    condition = re.sub("\\s+", " ", mapped_condition)
    condition = condition.strip()

    # Special case for WrapLibClang in qttools
    condition = condition.replace("TEST_libclang.has_clangcpp", "TEST_libclang")

    if has_failed:
        condition += " OR FIXME"

    return condition


def parseInput(ctx, sinput, data, cm_fh):
    skip_inputs = {
        "prefix",
        "hostprefix",
        "extprefix",
        "archdatadir",
        "bindir",
        "datadir",
        "docdir",
        "examplesdir",
        "external-hostbindir",
        "headerdir",
        "hostbindir",
        "hostdatadir",
        "hostlibdir",
        "importdir",
        "libdir",
        "libexecdir",
        "plugindir",
        "qmldir",
        "settingsdir",
        "sysconfdir",
        "testsdir",
        "translationdir",
        "android-arch",
        "android-ndk",
        "android-ndk-host",
        "android-ndk-platform",
        "android-sdk",
        "android-toolchain-version",
        "android-style-assets",
        "appstore-compliant",
        "avx",
        "avx2",
        "avx512",
        "c++std",
        "ccache",
        "commercial",
        "confirm-license",
        "dbus",
        "dbus-runtime",
        "debug",
        "debug-and-release",
        "developer-build",
        "device",
        "device-option",
        "f16c",
        "force-asserts",
        "force-debug-info",
        "force-pkg-config",
        "framework",
        "gc-binaries",
        "gdb-index",
        "gcc-sysroot",
        "gcov",
        "gnumake",
        "gui",
        "headersclean",
        "incredibuild-xge",
        "libudev",
        "ltcg",
        "make",
        "make-tool",
        "mips_dsp",
        "mips_dspr2",
        "mp",
        "nomake",
        "opensource",
        "optimize-debug",
        "optimize-size",
        "optimized-qmake",
        "optimized-tools",
        "pch",
        "pkg-config",
        "platform",
        "plugin-manifests",
        "profile",
        "qreal",
        "reduce-exports",
        "reduce-relocations",
        "release",
        "rpath",
        "sanitize",
        "sdk",
        "separate-debug-info",
        "shared",
        "silent",
        "qdbus",
        "sse2",
        "sse3",
        "sse4.1",
        "sse4.2",
        "ssse3",
        "static",
        "static-runtime",
        "strip",
        "syncqt",
        "sysroot",
        "testcocoon",
        "use-gold-linker",
        "warnings-are-errors",
        "Werror",
        "widgets",
        "xplatform",
        "zlib",
        "eventfd",
        "glib",
        "icu",
        "inotify",
        "journald",
        "pcre",
        "posix-ipc",
        "pps",
        "slog2",
        "syslog",
    }

    if sinput in skip_inputs:
        print(f"    **** Skipping input {sinput}: masked.")
        return

    dtype = data
    if isinstance(data, dict):
        dtype = data["type"]

    if dtype == "boolean":
        print(f"    **** Skipping boolean input {sinput}: masked.")
        return

    if dtype == "enum":
        values_line = " ".join(data["values"])
        cm_fh.write(f"# input {sinput}\n")
        cm_fh.write(f'set(INPUT_{featureName(sinput)} "undefined" CACHE STRING "")\n')
        cm_fh.write(
            f"set_property(CACHE INPUT_{featureName(sinput)} PROPERTY STRINGS undefined {values_line})\n\n"
        )
        return

    print(f"    XXXX UNHANDLED INPUT TYPE {dtype} in input description")
    return


def get_library_usage_for_compile_test(library):
    result = {}
    mapped_library = find_3rd_party_library_mapping(library)
    if not mapped_library:
        result["fixme"] = f"# FIXME: use: unmapped library: {library}\n"
        return result

    if mapped_library.test_library_overwrite:
        target_name = mapped_library.test_library_overwrite
    else:
        target_name = mapped_library.targetName
    result["target_name"] = target_name
    result["package_name"] = mapped_library.packageName
    result["extra"] = mapped_library.extra
    return result


# Handles config.test/foo/foo.pro projects.
def write_standalone_compile_test(cm_fh, ctx, data, config_test_name, is_library_test):
    rel_test_project_path = f"{ctx['test_dir']}/{config_test_name}"
    if posixpath.exists(f"{ctx['project_dir']}/{rel_test_project_path}/CMakeLists.txt"):
        label = ""
        libraries = []
        packages = []

        if "label" in data:
            label = data["label"]

        if is_library_test and config_test_name in data["libraries"]:
            if "label" in data["libraries"][config_test_name]:
                label = data["libraries"][config_test_name]["label"]

            # If a library entry in configure.json has a test, and
            # the test uses a config.tests standalone project, we
            # need to get the package and target info for the
            # library, and pass it to the test so compiling and
            # linking succeeds.
            library_usage = get_library_usage_for_compile_test(config_test_name)
            if "target_name" in library_usage:
                libraries.append(library_usage["target_name"])
            if "package_name" in library_usage:
                find_package_arguments = []
                find_package_arguments.append(library_usage["package_name"])
                if "extra" in library_usage:
                    find_package_arguments.extend(library_usage["extra"])
                package_line = "PACKAGE " + " ".join(find_package_arguments)
                packages.append(package_line)

        cm_fh.write(
            f"""
qt_config_compile_test("{config_test_name}"
                   LABEL "{label}"
                   PROJECT_PATH "${{CMAKE_CURRENT_SOURCE_DIR}}/{rel_test_project_path}"
"""
        )
        if libraries:
            libraries_string = " ".join(libraries)
            cm_fh.write(f"                   LIBRARIES {libraries_string}\n")
        if packages:
            packages_string = " ".join(packages)
            cm_fh.write(f"                   PACKAGES {packages_string}")
        cm_fh.write(")\n")


def write_compile_test(
    ctx, name, details, data, cm_fh, manual_library_list=None, is_library_test=False
):

    if manual_library_list is None:
        manual_library_list = []

    inherited_test_name = details["inherit"] if "inherit" in details else None
    inherit_details = None
    if inherited_test_name and is_library_test:
        inherit_details = data["libraries"][inherited_test_name]["test"]
        if not inherit_details:
            print(f"    XXXX Failed to locate inherited library test {inherited_test_name}")

    if isinstance(details, str):
        write_standalone_compile_test(cm_fh, ctx, data, details, is_library_test)
        return

    def resolve_head(detail):
        head = detail.get("head")
        if isinstance(head, list):
            head = "\n".join(head)
        return head + "\n" if head else ""

    head = ""
    if inherit_details:
        head += resolve_head(inherit_details)
    head += resolve_head(details)

    sourceCode = head

    def resolve_include(detail, keyword):
        include = detail.get(keyword, "")
        if isinstance(include, list):
            include = "#include <" + ">\n#include <".join(include) + ">\n"
        elif include:
            include = f"#include <{include}>\n"
        return include

    include = ""
    if is_library_test:
        if inherit_details:
            inherited_lib_data = data["libraries"][inherited_test_name]
            include += resolve_include(inherited_lib_data, "headers")
        this_lib_data = data["libraries"][name]
        include += resolve_include(this_lib_data, "headers")
    else:
        if inherit_details:
            include += resolve_include(inherit_details, "include")
        include += resolve_include(details, "include")

    sourceCode += include

    def resolve_tail(detail):
        tail = detail.get("tail")
        if isinstance(tail, list):
            tail = "\n".join(tail)
        return tail + "\n" if tail else ""

    tail = ""
    if inherit_details:
        tail += resolve_tail(inherit_details)
    tail += resolve_tail(details)

    sourceCode += tail

    if sourceCode:  # blank line before main
        sourceCode += "\n"
    sourceCode += "int main(void)\n"
    sourceCode += "{\n"
    sourceCode += "    /* BEGIN TEST: */\n"

    def resolve_main(detail):
        main = detail.get("main")
        if isinstance(main, list):
            main = "\n".join(main)
        return main + "\n" if main else ""

    main = ""
    if inherit_details:
        main += resolve_main(inherit_details)
    main += resolve_main(details)

    sourceCode += main

    sourceCode += "    /* END TEST: */\n"
    sourceCode += "    return 0;\n"
    sourceCode += "}\n"

    sourceCode = sourceCode.replace('"', '\\"')

    librariesCmakeName = ""
    languageStandard = ""
    compileOptions = ""
    qmakeFixme = ""

    cm_fh.write(f"# {name}\n")

    if "qmake" in details:  # We don't really have many so we can just enumerate them all
        if details["qmake"] == "unix:LIBS += -lpthread":
            librariesCmakeName = format(featureName(name)) + "_TEST_LIBRARIES"
            cm_fh.write("if (UNIX)\n")
            cm_fh.write("    set(" + librariesCmakeName + " pthread)\n")
            cm_fh.write("endif()\n")
        elif details["qmake"] == "linux: LIBS += -lpthread -lrt":
            librariesCmakeName = format(featureName(name)) + "_TEST_LIBRARIES"
            cm_fh.write("if (LINUX)\n")
            cm_fh.write("    set(" + librariesCmakeName + " pthread rt)\n")
            cm_fh.write("endif()\n")
        elif details["qmake"] == "!winrt: LIBS += runtimeobject.lib":
            librariesCmakeName = format(featureName(name)) + "_TEST_LIBRARIES"
            cm_fh.write("if (NOT WINRT)\n")
            cm_fh.write("    set(" + librariesCmakeName + " runtimeobject)\n")
            cm_fh.write("endif()\n")
        elif details["qmake"] == "CONFIG += c++11":
            # do nothing we're always in c++11 mode
            pass
        elif details["qmake"] == "CONFIG += c++11 c++14":
            languageStandard = "CXX_STANDARD 14"
        elif details["qmake"] == "CONFIG += c++11 c++14 c++17":
            languageStandard = "CXX_STANDARD 17"
        elif details["qmake"] == "CONFIG += c++11 c++14 c++17 c++20":
            languageStandard = "CXX_STANDARD 20"
        elif details["qmake"] == "QMAKE_CXXFLAGS += -fstack-protector-strong":
            compileOptions = details["qmake"][18:]
        else:
            qmakeFixme = f"# FIXME: qmake: {details['qmake']}\n"

    library_list = []
    test_libraries = manual_library_list

    if "use" in data:
        test_libraries += data["use"].split(" ")

    for library in test_libraries:
        if len(library) == 0:
            continue

        adjusted_library = get_compile_test_dependent_library_mapping(name, library)
        library_usage = get_library_usage_for_compile_test(adjusted_library)
        if "fixme" in library_usage:
            qmakeFixme += library_usage["fixme"]
            continue
        else:
            library_list.append(library_usage["target_name"])

    cm_fh.write(f"qt_config_compile_test({featureName(name)}\n")
    cm_fh.write(lineify("LABEL", data.get("label", "")))
    if librariesCmakeName != "" or len(library_list) != 0:
        cm_fh.write("    LIBRARIES\n")
        if librariesCmakeName != "":
            cm_fh.write(lineify("", "${" + librariesCmakeName + "}"))
        if len(library_list) != 0:
            cm_fh.write("        ")
            cm_fh.write("\n        ".join(library_list))
            cm_fh.write("\n")
    if compileOptions != "":
        cm_fh.write(f"    COMPILE_OPTIONS {compileOptions}\n")
    cm_fh.write("    CODE\n")
    cm_fh.write('"' + sourceCode + '"')
    if qmakeFixme != "":
        cm_fh.write(qmakeFixme)
    if languageStandard != "":
        cm_fh.write(f"\n    {languageStandard}\n")
    cm_fh.write(")\n\n")


#  "tests": {
#        "cxx11_future": {
#            "label": "C++11 <future>",
#            "type": "compile",
#            "test": {
#                "include": "future",
#                "main": [
#                    "std::future<int> f = std::async([]() { return 42; });",
#                    "(void)f.get();"
#                ],
#                "qmake": "unix:LIBS += -lpthread"
#            }
#        },


def write_compiler_supports_flag_test(
    ctx, name, details, data, cm_fh, manual_library_list=None, is_library_test=False
):
    cm_fh.write(f"qt_config_compiler_supports_flag_test({featureName(name)}\n")
    cm_fh.write(lineify("LABEL", data.get("label", "")))
    cm_fh.write(lineify("FLAG", data.get("flag", "")))
    cm_fh.write(")\n\n")


def write_linker_supports_flag_test(
    ctx, name, details, data, cm_fh, manual_library_list=None, is_library_test=False
):
    cm_fh.write(f"qt_config_linker_supports_flag_test({featureName(name)}\n")
    cm_fh.write(lineify("LABEL", data.get("label", "")))
    cm_fh.write(lineify("FLAG", data.get("flag", "")))
    cm_fh.write(")\n\n")


def parseTest(ctx, test, data, cm_fh):
    skip_tests = {
        "c11",
        "c99",
        "gc_binaries",
        "precomile_header",
        "reduce_exports",
        "gc_binaries",
        "libinput_axis_api",
        "wayland-scanner",
        "xlib",
    }

    if test in skip_tests:
        print(f"    **** Skipping features {test}: masked.")
        return

    if data["type"] == "compile":
        knownTests.add(test)

        if "test" in data:
            details = data["test"]
        else:
            details = test

        write_compile_test(ctx, test, details, data, cm_fh)

    if data["type"] == "compilerSupportsFlag":
        knownTests.add(test)

        if "test" in data:
            details = data["test"]
        else:
            details = test

        write_compiler_supports_flag_test(ctx, test, details, data, cm_fh)

    if data["type"] == "linkerSupportsFlag":
        knownTests.add(test)

        if "test" in data:
            details = data["test"]
        else:
            details = test

        write_linker_supports_flag_test(ctx, test, details, data, cm_fh)

    elif data["type"] == "libclang":
        knownTests.add(test)

        cm_fh.write(f"# {test}\n")
        lib_clang_lib = find_3rd_party_library_mapping("libclang")
        cm_fh.write(generate_find_package_info(lib_clang_lib))
        cm_fh.write(
            dedent(
                """
        if(TARGET WrapLibClang::WrapLibClang)
            set(TEST_libclang "ON" CACHE BOOL "Required libclang version found." FORCE)
        endif()
        """
            )
        )
        cm_fh.write("\n")

    elif data["type"] == "x86Simd":
        knownTests.add(test)

        label = data["label"]

        cm_fh.write(f"# {test}\n")
        cm_fh.write(f'qt_config_compile_test_x86simd({test} "{label}")\n')
        cm_fh.write("\n")

    elif data["type"] == "machineTuple":
        knownTests.add(test)

        label = data["label"]

        cm_fh.write(f"# {test}\n")
        cm_fh.write(f'qt_config_compile_test_machine_tuple("{label}")\n')
        cm_fh.write("\n")

    #    "features": {
    #        "android-style-assets": {
    #            "label": "Android Style Assets",
    #            "condition": "config.android",
    #            "output": [ "privateFeature" ],
    #            "comment": "This belongs into gui, but the license check needs it here already."
    #        },
    else:
        print(f"    XXXX UNHANDLED TEST TYPE {data['type']} in test description")


def get_feature_mapping():
    # This is *before* the feature name gets normalized! So keep - and + chars, etc.
    feature_mapping = {
        "alloc_h": None,  # handled by alloc target
        "alloc_malloc_h": None,
        "alloc_stdlib_h": None,
        "build_all": None,
        "ccache": {"autoDetect": "1", "condition": "QT_USE_CCACHE"},
        "compiler-flags": None,
        "cross_compile": {"condition": "CMAKE_CROSSCOMPILING"},
        "debug_and_release": {
            "autoDetect": "1",  # Setting this to None has weird effects...
            "condition": "QT_GENERATOR_IS_MULTI_CONFIG",
        },
        "debug": {
            "autoDetect": "ON",
            "condition": "CMAKE_BUILD_TYPE STREQUAL Debug OR Debug IN_LIST CMAKE_CONFIGURATION_TYPES",
        },
        "dlopen": {"condition": "UNIX"},
        "force_debug_info": {
            "autoDetect": "CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo OR RelWithDebInfo IN_LIST CMAKE_CONFIGURATION_TYPES"
        },
        "framework": {
            "condition": "APPLE AND BUILD_SHARED_LIBS AND NOT CMAKE_BUILD_TYPE STREQUAL Debug"
        },
        "gc_binaries": {"condition": "NOT QT_FEATURE_shared"},
        "gcc-sysroot": None,
        "gcov": None,
        "GNUmake": None,
        "host-dbus": None,
        "iconv": {
            "condition": "NOT QT_FEATURE_icu AND QT_FEATURE_textcodec AND NOT WIN32 AND NOT QNX AND NOT ANDROID AND NOT APPLE AND WrapIconv_FOUND",
        },
        "incredibuild_xge": None,
        "ltcg": {
            "autoDetect": "ON",
            "cmakePrelude": """set(__qt_ltcg_detected FALSE)
if(CMAKE_INTERPROCEDURAL_OPTIMIZATION)
    set(__qt_ltcg_detected TRUE)
else()
    foreach(config ${CMAKE_BUILD_TYPE} ${CMAKE_CONFIGURATION_TYPES})
        string(TOUPPER "${config}" __qt_uc_config)
        if(CMAKE_INTERPROCEDURAL_OPTIMIZATION_${__qt_uc_config})
            set(__qt_ltcg_detected TRUE)
            break()
        endif()
    endforeach()
    unset(__qt_uc_config)
endif()""",
            "condition": "__qt_ltcg_detected",
        },
        "msvc_mp": None,
        "simulator_and_device": {"condition": "UIKIT AND NOT QT_UIKIT_SDK"},
        "pkg-config": {"condition": "PKG_CONFIG_FOUND"},
        "precompile_header": {"condition": "BUILD_WITH_PCH"},
        "profile": None,
        "qmakeargs": None,
        "qpa_default_platform": None,  # Not a bool!
        "qreal": {
            "condition": 'DEFINED QT_COORD_TYPE AND NOT QT_COORD_TYPE STREQUAL "double"',
            "output": [
                {
                    "type": "define",
                    "name": "QT_COORD_TYPE",
                    "value": "${QT_COORD_TYPE}",
                },
                {
                    "type": "define",
                    "name": "QT_COORD_TYPE_STRING",
                    "value": '\\"${QT_COORD_TYPE}\\"',
                },
            ],
        },
        "reduce_exports": {
            "condition": "NOT MSVC",
        },
        "release": None,
        "release_tools": None,
        "rpath": {
            "autoDetect": "1",
            "condition": "BUILD_SHARED_LIBS AND UNIX AND NOT WIN32 AND NOT ANDROID",
        },
        "shared": {
            "condition": "BUILD_SHARED_LIBS",
            "output": [
                "publicFeature",
                "publicQtConfig",
                "publicConfig",
                {
                    "type": "define",
                    "name": "QT_STATIC",
                    "prerequisite": "!defined(QT_SHARED) && !defined(QT_STATIC)",
                    "negative": True,
                },
            ],
        },
        "silent": None,
        "sql-sqlite": {"condition": "QT_FEATURE_datestring"},
        "stl": None,  # Do we really need to test for this in 2018?!
        "strip": None,
        "verifyspec": None,  # qmake specific...
        "warnings_are_errors": None,  # FIXME: Do we need these?
        "xkbcommon-system": None,  # another system library, just named a bit different from the rest
    }
    return feature_mapping


def parseFeature(ctx, feature, data, cm_fh):
    feature_mapping = get_feature_mapping()
    mapping = feature_mapping.get(feature, {})

    if mapping is None:
        print(f"    **** Skipping features {feature}: masked.")
        return

    handled = {
        "autoDetect",
        "comment",
        "condition",
        "description",
        "disable",
        "emitIf",
        "enable",
        "label",
        "output",
        "purpose",
        "section",
    }
    label = mapping.get("label", data.get("label", ""))
    purpose = mapping.get("purpose", data.get("purpose", data.get("description", label)))
    autoDetect = map_condition(mapping.get("autoDetect", data.get("autoDetect", "")))
    condition = map_condition(mapping.get("condition", data.get("condition", "")))
    output = mapping.get("output", data.get("output", []))
    comment = mapping.get("comment", data.get("comment", ""))
    section = mapping.get("section", data.get("section", ""))
    enable = map_condition(mapping.get("enable", data.get("enable", "")))
    disable = map_condition(mapping.get("disable", data.get("disable", "")))
    emitIf = map_condition(mapping.get("emitIf", data.get("emitIf", "")))
    cmakePrelude = mapping.get("cmakePrelude", None)
    cmakeEpilogue = mapping.get("cmakeEpilogue", None)

    for k in [k for k in data.keys() if k not in handled]:
        print(f"    XXXX UNHANDLED KEY {k} in feature description")

    if not output:
        # feature that is only used in the conditions of other features
        output = ["internalFeature"]

    publicFeature = False  # #define QT_FEATURE_featurename in public header
    privateFeature = False  # #define QT_FEATURE_featurename in private header
    negativeFeature = False  # #define QT_NO_featurename in public header
    internalFeature = False  # No custom or QT_FEATURE_ defines
    publicDefine = False  # #define MY_CUSTOM_DEFINE in public header
    publicConfig = False  # add to CONFIG in public pri file
    privateConfig = False  # add to CONFIG in private pri file
    publicQtConfig = False  # add to QT_CONFIG in public pri file

    for o in output:
        outputType = o
        if isinstance(o, dict):
            outputType = o["type"]

        if outputType in [
            "varAssign",
            "varAppend",
            "varRemove",
            "useBFDLinker",
            "useGoldLinker",
            "useLLDLinker",
        ]:
            continue
        elif outputType == "define":
            publicDefine = True
        elif outputType == "feature":
            negativeFeature = True
        elif outputType == "publicFeature":
            publicFeature = True
        elif outputType == "privateFeature":
            privateFeature = True
        elif outputType == "internalFeature":
            internalFeature = True
        elif outputType == "publicConfig":
            publicConfig = True
        elif outputType == "privateConfig":
            privateConfig = True
        elif outputType == "publicQtConfig":
            publicQtConfig = True
        else:
            print(f"    XXXX UNHANDLED OUTPUT TYPE {outputType} in feature {feature}.")
            continue

    if not any(
        [
            publicFeature,
            privateFeature,
            internalFeature,
            publicDefine,
            negativeFeature,
            publicConfig,
            privateConfig,
            publicQtConfig,
        ]
    ):
        print(f"    **** Skipping feature {feature}: Not relevant for C++.")
        return

    normalized_feature_name = featureName(feature)

    def writeFeature(
        name,
        publicFeature=False,
        privateFeature=False,
        labelAppend="",
        superFeature=None,
        autoDetect="",
        cmakePrelude=None,
        cmakeEpilogue=None,
    ):
        if comment:
            cm_fh.write(f"# {comment}\n")

        if cmakePrelude is not None:
            cm_fh.write(cmakePrelude)
            cm_fh.write("\n")

        cm_fh.write(f'qt_feature("{name}"')
        if publicFeature:
            cm_fh.write(" PUBLIC")
        if privateFeature:
            cm_fh.write(" PRIVATE")
        cm_fh.write("\n")

        cm_fh.write(lineify("SECTION", section))
        cm_fh.write(lineify("LABEL", label + labelAppend))
        if purpose != label:
            cm_fh.write(lineify("PURPOSE", purpose))
        cm_fh.write(lineify("AUTODETECT", autoDetect, quote=False))
        if superFeature:
            feature_condition = f"QT_FEATURE_{superFeature}"
        else:
            feature_condition = condition
        cm_fh.write(lineify("CONDITION", feature_condition, quote=False))
        cm_fh.write(lineify("ENABLE", enable, quote=False))
        cm_fh.write(lineify("DISABLE", disable, quote=False))
        cm_fh.write(lineify("EMIT_IF", emitIf, quote=False))
        cm_fh.write(")\n")

        if cmakeEpilogue is not None:
            cm_fh.write(cmakeEpilogue)
            cm_fh.write("\n")

    # Write qt_feature() calls before any qt_feature_definition() calls

    # Default internal feature case.
    featureCalls = {}
    featureCalls[feature] = {
        "name": feature,
        "labelAppend": "",
        "autoDetect": autoDetect,
        "cmakePrelude": cmakePrelude,
        "cmakeEpilogue": cmakeEpilogue,
    }

    # Go over all outputs to compute the number of features that have to be declared
    for o in output:
        outputType = o
        name = feature

        # The label append is to provide a unique label for features that have more than one output
        # with different names.
        labelAppend = ""

        if isinstance(o, dict):
            outputType = o["type"]
            if "name" in o:
                name = o["name"]
                labelAppend = f": {o['name']}"

        if outputType not in ["feature", "publicFeature", "privateFeature"]:
            continue
        if name not in featureCalls:
            featureCalls[name] = {"name": name, "labelAppend": labelAppend}

        if name != feature:
            featureCalls[name]["superFeature"] = normalized_feature_name

        if outputType in ["feature", "publicFeature"]:
            featureCalls[name]["publicFeature"] = True
        elif outputType == "privateFeature":
            featureCalls[name]["privateFeature"] = True
        elif outputType == "publicConfig":
            featureCalls[name]["publicConfig"] = True
        elif outputType == "privateConfig":
            featureCalls[name]["privateConfig"] = True
        elif outputType == "publicQtConfig":
            featureCalls[name]["publicQtConfig"] = True

    # Write the qt_feature() calls from the computed feature map
    for _, args in featureCalls.items():
        writeFeature(**args)

    # Write qt_feature_definition() calls
    for o in output:
        outputType = o
        outputArgs = {}
        if isinstance(o, dict):
            outputType = o["type"]
            outputArgs = o

        # Map negative feature to define:
        if outputType == "feature":
            outputType = "define"
            outputArgs = {
                "name": f"QT_NO_{normalized_feature_name.upper()}",
                "negative": True,
                "value": 1,
                "type": "define",
            }

        if outputType != "define":
            continue

        if outputArgs.get("name") is None:
            print(f"    XXXX DEFINE output without name in feature {feature}.")
            continue

        out_name = outputArgs.get("name")
        cm_fh.write(f'qt_feature_definition("{feature}" "{out_name}"')
        if outputArgs.get("negative", False):
            cm_fh.write(" NEGATE")
        if outputArgs.get("value") is not None:
            cm_fh.write(f' VALUE "{outputArgs.get("value")}"')
        if outputArgs.get("prerequisite") is not None:
            cm_fh.write(f' PREREQUISITE "{outputArgs.get("prerequisite")}"')
        cm_fh.write(")\n")

    # Write qt_feature_config() calls
    for o in output:
        outputType = o
        name = feature
        modified_name = name

        outputArgs = {}
        if isinstance(o, dict):
            outputType = o["type"]
            outputArgs = o
            if "name" in o:
                modified_name = o["name"]

        if outputType not in ["publicConfig", "privateConfig", "publicQtConfig"]:
            continue

        config_type = ""
        if outputType == "publicConfig":
            config_type = "QMAKE_PUBLIC_CONFIG"
        elif outputType == "privateConfig":
            config_type = "QMAKE_PRIVATE_CONFIG"
        elif outputType == "publicQtConfig":
            config_type = "QMAKE_PUBLIC_QT_CONFIG"

        if not config_type:
            print("    XXXX config output without type in feature {}.".format(feature))
            continue

        cm_fh.write('qt_feature_config("{}" {}'.format(name, config_type))
        if outputArgs.get("negative", False):
            cm_fh.write("\n    NEGATE")
        if modified_name != name:
            cm_fh.write("\n")
            cm_fh.write(lineify("NAME", modified_name, quote=True))

        cm_fh.write(")\n")


def processSummaryHelper(ctx, entries, cm_fh):
    for entry in entries:
        if isinstance(entry, str):
            name = entry
            cm_fh.write(f'qt_configure_add_summary_entry(ARGS "{name}")\n')
        elif "type" in entry and entry["type"] in [
            "feature",
            "firstAvailableFeature",
            "featureList",
        ]:
            function_args = []
            entry_type = entry["type"]

            if entry_type in ["firstAvailableFeature", "featureList"]:
                feature_mapping = get_feature_mapping()
                unhandled_feature = False
                for feature_name, value in feature_mapping.items():
                    # Skip entries that mention a feature which is
                    # skipped by configurejson2cmake in the feature
                    # mapping. This is not ideal, but prevents errors at
                    # CMake configuration time.
                    if not value and f"{feature_name}" in entry["args"]:
                        unhandled_feature = True
                        break

                if unhandled_feature:
                    print(f"    XXXX UNHANDLED FEATURE in SUMMARY TYPE {entry}.")
                    continue

            if entry_type != "feature":
                function_args.append(lineify("TYPE", entry_type))
            if "args" in entry:
                args = entry["args"]
                function_args.append(lineify("ARGS", args))
            if "message" in entry:
                message = entry["message"]
                function_args.append(lineify("MESSAGE", message))
            if "condition" in entry:
                condition = map_condition(entry["condition"])
                function_args.append(lineify("CONDITION", condition, quote=False))
            entry_args_string = "".join(function_args)
            cm_fh.write(f"qt_configure_add_summary_entry(\n{entry_args_string})\n")
        elif "type" in entry and entry["type"] == "buildTypeAndConfig":
            cm_fh.write("qt_configure_add_summary_build_type_and_config()\n")
        elif "type" in entry and entry["type"] == "buildMode":
            message = entry["message"]
            cm_fh.write(f"qt_configure_add_summary_build_mode({message})\n")
        elif "type" in entry and entry["type"] == "buildParts":
            message = entry["message"]
            cm_fh.write(f'qt_configure_add_summary_build_parts("{message}")\n')
        elif "section" in entry:
            section = entry["section"]
            cm_fh.write(f'qt_configure_add_summary_section(NAME "{section}")\n')
            processSummaryHelper(ctx, entry["entries"], cm_fh)
            cm_fh.write(f'qt_configure_end_summary_section() # end of "{section}" section\n')
        else:
            print(f"    XXXX UNHANDLED SUMMARY TYPE {entry}.")


report_condition_mapping = {
    "(features.rpath || features.rpath_dir) && !features.shared": "(features.rpath || QT_EXTRA_RPATHS) && !features.shared",
    "(features.rpath || features.rpath_dir) && var.QMAKE_LFLAGS_RPATH == ''": None,
}


def processReportHelper(ctx, entries, cm_fh):
    feature_mapping = get_feature_mapping()

    for entry in entries:
        if isinstance(entry, dict):
            entry_args = []
            if "type" not in entry:
                print(f"    XXXX UNHANDLED REPORT TYPE missing type in {entry}.")
                continue

            report_type = entry["type"]
            if report_type not in ["note", "warning", "error"]:
                print(f"    XXXX UNHANDLED REPORT TYPE unknown type in {entry}.")
                continue

            report_type = report_type.upper()
            entry_args.append(lineify("TYPE", report_type, quote=False))
            message = entry["message"]

            # Replace semicolons, qt_parse_all_arguments can't handle
            # them due to an escaping bug in CMake regarding escaping
            # macro arguments.
            # https://gitlab.kitware.com/cmake/cmake/issues/19972
            message = message.replace(";", ",")

            entry_args.append(lineify("MESSAGE", message))
            # Need to overhaul everything to fix conditions.
            if "condition" in entry:
                condition = entry["condition"]

                unhandled_condition = False
                for feature_name, value in feature_mapping.items():
                    # Skip reports that mention a feature which is
                    # skipped by configurejson2cmake in the feature
                    # mapping. This is not ideal, but prevents errors at
                    # CMake configuration time.
                    if not value and f"features.{feature_name}" in condition:
                        unhandled_condition = True
                        break

                if unhandled_condition:
                    print(f"    XXXX UNHANDLED CONDITION in REPORT TYPE {entry}.")
                    continue

                if isinstance(condition, str) and condition in report_condition_mapping:
                    new_condition = report_condition_mapping[condition]
                    if new_condition is None:
                        continue
                    else:
                        condition = new_condition
                condition = map_condition(condition)
                entry_args.append(lineify("CONDITION", condition, quote=False))
            entry_args_string = "".join(entry_args)
            cm_fh.write(f"qt_configure_add_report_entry(\n{entry_args_string})\n")
        else:
            print(f"    XXXX UNHANDLED REPORT TYPE {entry}.")


def parseCommandLineCustomHandler(ctx, data, cm_fh):
    cm_fh.write(f"qt_commandline_custom({data})\n")


def parseCommandLineOptions(ctx, data, cm_fh):
    for key in data:
        args = [key]
        option = data[key]
        if isinstance(option, str):
            args += ["TYPE", option]
        else:
            if "type" in option:
                args += ["TYPE", option["type"]]
            if "name" in option:
                args += ["NAME", option["name"]]
            if "value" in option:
                args += ["VALUE", option["value"]]
            if "values" in option:
                values = option["values"]
                if isinstance(values, list):
                    args += ["VALUES", " ".join(option["values"])]
                else:
                    args += ["MAPPING"]
                    for lhs in values:
                        args += [lhs, values[lhs]]

        cm_fh.write(f"qt_commandline_option({' '.join(args)})\n")


def parseCommandLinePrefixes(ctx, data, cm_fh):
    for key in data:
        cm_fh.write(f"qt_commandline_prefix({key} {data[key]})\n")


def processCommandLine(ctx, data, cm_fh):
    print("  commandline:")

    if "subconfigs" in data:
        for subconf in data["subconfigs"]:
            cm_fh.write(f"qt_commandline_subconfig({subconf})\n")

    if "commandline" not in data:
        return

    commandLine = data["commandline"]
    if "custom" in commandLine:
        print("    custom:")
        parseCommandLineCustomHandler(ctx, commandLine["custom"], cm_fh)
    if "options" in commandLine:
        print("    options:")
        parseCommandLineOptions(ctx, commandLine["options"], cm_fh)
    if "prefix" in commandLine:
        print("    prefix:")
        parseCommandLinePrefixes(ctx, commandLine["prefix"], cm_fh)
    if "assignments" in commandLine:
        print("    assignments are ignored")


def processInputs(ctx, data, cm_fh):
    print("  inputs:")
    if "commandline" not in data:
        return

    commandLine = data["commandline"]
    if "options" not in commandLine:
        return

    for input_option in commandLine["options"]:
        parseInput(ctx, input_option, commandLine["options"][input_option], cm_fh)


def processTests(ctx, data, cm_fh):
    print("  tests:")
    if "tests" not in data:
        return

    for test in data["tests"]:
        parseTest(ctx, test, data["tests"][test], cm_fh)


def processFeatures(ctx, data, cm_fh):
    print("  features:")
    if "features" not in data:
        return

    for feature in data["features"]:
        parseFeature(ctx, feature, data["features"][feature], cm_fh)


def processLibraries(ctx, data, cm_fh):
    cmake_find_packages_set = set()
    print("  libraries:")
    if "libraries" not in data:
        return

    for lib in data["libraries"]:
        parseLib(ctx, lib, data, cm_fh, cmake_find_packages_set)


def processReports(ctx, data, cm_fh):
    if "summary" in data:
        print("  summary:")
        processSummaryHelper(ctx, data["summary"], cm_fh)
    if "report" in data:
        print("  report:")
        processReportHelper(ctx, data["report"], cm_fh)
    if "earlyReport" in data:
        print("  earlyReport:")
        processReportHelper(ctx, data["earlyReport"], cm_fh)


def processSubconfigs(path, ctx, data):
    assert ctx is not None
    if "subconfigs" in data:
        for subconf in data["subconfigs"]:
            subconfDir = posixpath.join(path, subconf)
            subconfData = readJsonFromDir(subconfDir)
            subconfCtx = ctx
            processJson(subconfDir, subconfCtx, subconfData)


class special_cased_file:
    def __init__(self, base_dir: str, file_name: str, skip_special_case_preservation: bool):
        self.base_dir = base_dir
        self.file_path = posixpath.join(base_dir, file_name)
        self.gen_file_path = self.file_path + ".gen"
        self.preserve_special_cases = not skip_special_case_preservation

    def __enter__(self):
        self.file = open(self.gen_file_path, "w")
        if self.preserve_special_cases:
            self.sc_handler = SpecialCaseHandler(
                os.path.abspath(self.file_path),
                os.path.abspath(self.gen_file_path),
                os.path.abspath(self.base_dir),
                debug=False,
            )
        return self.file

    def __exit__(self, type, value, trace_back):
        self.file.close()
        if self.preserve_special_cases:
            self.sc_handler.handle_special_cases()
        os.replace(self.gen_file_path, self.file_path)


def processJson(path, ctx, data, skip_special_case_preservation=False):
    ctx["project_dir"] = path
    ctx["module"] = data.get("module", "global")
    ctx["test_dir"] = data.get("testDir", "config.tests")

    ctx = processFiles(ctx, data)

    with special_cased_file(path, "qt_cmdline.cmake", skip_special_case_preservation) as cm_fh:
        processCommandLine(ctx, data, cm_fh)

    with special_cased_file(path, "configure.cmake", skip_special_case_preservation) as cm_fh:
        cm_fh.write("\n\n#### Inputs\n\n")

        processInputs(ctx, data, cm_fh)

        cm_fh.write("\n\n#### Libraries\n\n")

        processLibraries(ctx, data, cm_fh)

        cm_fh.write("\n\n#### Tests\n\n")

        processTests(ctx, data, cm_fh)

        cm_fh.write("\n\n#### Features\n\n")

        processFeatures(ctx, data, cm_fh)

        processReports(ctx, data, cm_fh)

        if ctx.get("module") == "global":
            cm_fh.write(
                '\nqt_extra_definition("QT_VERSION_STR" "\\"${PROJECT_VERSION}\\"" PUBLIC)\n'
            )
            cm_fh.write('qt_extra_definition("QT_VERSION_MAJOR" ${PROJECT_VERSION_MAJOR} PUBLIC)\n')
            cm_fh.write('qt_extra_definition("QT_VERSION_MINOR" ${PROJECT_VERSION_MINOR} PUBLIC)\n')
            cm_fh.write('qt_extra_definition("QT_VERSION_PATCH" ${PROJECT_VERSION_PATCH} PUBLIC)\n')

    # do this late:
    processSubconfigs(path, ctx, data)


def main():
    if len(sys.argv) < 2:
        print("This scripts needs one directory to process!")
        quit(1)

    directory = sys.argv[1]
    skip_special_case_preservation = "-s" in sys.argv[2:]

    print(f"Processing: {directory}.")

    data = readJsonFromDir(directory)
    processJson(directory, {}, data, skip_special_case_preservation=skip_special_case_preservation)


if __name__ == "__main__":
    main()
