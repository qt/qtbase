# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16...3.28)

project(test_cxx_modules_moc_lowlevel)

# Unlike the sibling test_cxx_modules_moc, which exercises moc via AUTOMOC the way an end user
# eventually will (once upstream CMake gains AUTOMOC support for FILE_SET CXX_MODULES sources),
# this test invokes moc on the module units directly via qt_generate_moc(), and its main.cpp
# makes actual assertions about the resulting QMetaObjects instead of just relying on the
# program compiling and linking. It can therefore run today, without waiting on that upstream
# support.

# C++ module support requires CMake 3.28+; skip silently otherwise.
if(CMAKE_VERSION VERSION_LESS "3.28")
    return()
endif()

# Ninja 1.11+ is also required for C++ module support; skip silently otherwise.
execute_process(
    COMMAND "${CMAKE_MAKE_PROGRAM}" --version
    OUTPUT_VARIABLE _ninja_version
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET
)
if(_ninja_version VERSION_LESS "1.11")
    return()
endif()

# Qt headers pulled in via Q_OBJECT (e.g. qobject_impl.h) currently trigger exposure
# violations when compiled as part of a module unit with GCC (QTBUG-142513).
# Skip until that's fixed.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Core Test)

qt_add_executable(test_cxx_modules_moc_lowlevel main.cpp mod-test-helpers.cpp)
# qt_add_executable() is defined inside Qt6CoreMacros.cmake, which Qt6Config.cmake loads under
# its own "cmake_minimum_required(VERSION 3.16...3.21)". That caps the policy version recorded
# in the function at 3.21, which is below CMake 3.28's threshold for automatically scanning
# ordinary sources for C++ module dependencies. Setting CXX_SCAN_FOR_MODULES explicitly
# overrides that default and opts all sources in this target into module dependency scanning.
set_target_properties(test_cxx_modules_moc_lowlevel PROPERTIES CXX_SCAN_FOR_MODULES ON)

target_sources(test_cxx_modules_moc_lowlevel
    PRIVATE FILE_SET CXX_MODULES FILES
        mod.cppm
        mod-part.cppm
        mod-internal.cppm
        helpermod.cppm
)

set_target_properties(test_cxx_modules_moc_lowlevel PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    AUTOMOC ON
)

target_link_libraries(test_cxx_modules_moc_lowlevel PRIVATE Qt::Core Qt::Test)

# AUTOMOC only ever sees main.cpp: the module interface/partition units below are excluded
# from it via qt_generate_moc() (see the TARGET argument's SKIP_AUTOMOC side effect), and
# nothing else here contains a meta-object macro. qt_wrap_cpp() isn't usable for the module
# units: since CMake registers .cppm as a C++ source extension (not a header one), it would
# treat them as needing a "#include \"x.moc\"" in the same file, which doesn't work for a
# module interface/partition unit. qt_generate_moc() is the lower-level primitive that lets the
# output file be named explicitly instead, sidestepping that issue.
foreach(module_unit_source mod.cppm mod-part.cppm mod-internal.cppm)
    get_filename_component(module_unit_name "${module_unit_source}" NAME_WE)
    set(moc_output "${CMAKE_CURRENT_BINARY_DIR}/moc_${module_unit_name}.cpp")
    qt6_generate_moc("${module_unit_source}" "${moc_output}" TARGET test_cxx_modules_moc_lowlevel)
    target_sources(test_cxx_modules_moc_lowlevel PRIVATE "${moc_output}")
endforeach()

# main.cpp makes real assertions about the generated QMetaObjects and exits with a failure
# code if any of them are wrong; run it as part of the build itself (rather than registering
# a separate ctest BINARY to run afterwards) so that skipping this whole project on toolchains
# that don't meet the requirements above (nothing to build) is still correctly treated as a
# pass, exactly like the sibling tests.
add_custom_command(TARGET test_cxx_modules_moc_lowlevel POST_BUILD
    COMMAND test_cxx_modules_moc_lowlevel
    COMMENT "Running test_cxx_modules_moc_lowlevel to verify the generated QMetaObjects"
    VERBATIM
)
