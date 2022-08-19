# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

message(STATUS "Entered subdir_with_global_qt subdirectory")

set(file_path "${CMAKE_CURRENT_BINARY_DIR}/lib.cpp")
file(GENERATE OUTPUT "${file_path}" CONTENT "int foo() { return 42; }")
add_library(lib_global_qt STATIC "${file_path}")

# These Qt targets will be available in all scopes of the project.
# The previous local targets are simply shadowed.
set(QT_PROMOTE_TO_GLOBAL_TARGETS ON)
find_package(Qt6 REQUIRED COMPONENTS Gui)

target_link_libraries(lib_global_qt PRIVATE Qt6::Gui)

message(STATUS "Exiting subdir_with_global_qt subdirectory")
