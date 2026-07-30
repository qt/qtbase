# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16...3.28)

project(test_cxx_modules_moc)

# TODO: Lower this to the actual CMake version once it adds AUTOMOC support for C++ module
# interface and partition units. Right now, AUTOMOC treats FILE_SET CXX_MODULES sources like
# plain .cpp sources and demands a manual "#include <name>.moc", which doesn't work for module
# units. That version will necessarily be >= 3.28, the version that introduced FILE_SET
# CXX_MODULES in the first place, so no separate check for that is needed here.
if(CMAKE_VERSION VERSION_LESS "100.0")
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
