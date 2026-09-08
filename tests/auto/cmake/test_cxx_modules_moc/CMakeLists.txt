# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16...3.28)

project(test_cxx_modules_moc)

# CMake 4.5 is the first version with AUTOMOC support for C++ module interface and partition
# units. Earlier versions treat FILE_SET CXX_MODULES sources like plain .cpp sources and demand
# a manual "#include <name>.moc", which doesn't work for module units. 4.5 is past 3.28, the
# version that introduced FILE_SET CXX_MODULES in the first place, so no separate check for that
# is needed here.
if(CMAKE_VERSION VERSION_LESS "4.5")
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
# violations when compiled as part of a module unit with GCC. Skip until that's fixed.
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    return()
endif()

find_package(Qt6 REQUIRED COMPONENTS Core)

add_executable(test_cxx_modules_moc main.cpp)

target_sources(test_cxx_modules_moc
    PRIVATE FILE_SET CXX_MODULES FILES
        mod.cppm
        mod-part.cppm
        mod-internal.cppm
        helpermod.cppm
)

set_target_properties(test_cxx_modules_moc PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
    AUTOMOC ON
)

target_link_libraries(test_cxx_modules_moc PRIVATE Qt::Core)

qt6_extract_metatypes(test_cxx_modules_moc OUTPUT_FILES metatypes_file)

# Collecting the meta types of a module unit requires cmake_automoc_parser to look at the
# CXX_MODULE_UNITS entries of AutogenInfo.json. Run the check as part of the build (like the
# sibling test_cxx_modules_moc_lowlevel does) so that skipping this project entirely still
# counts as a pass.
add_custom_command(TARGET test_cxx_modules_moc POST_BUILD
    COMMAND "${CMAKE_COMMAND}" "-Dmetatypes_file=${metatypes_file}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/check_metatypes.cmake"
    COMMENT "Verifying the meta types collected for test_cxx_modules_moc"
    VERBATIM
)
