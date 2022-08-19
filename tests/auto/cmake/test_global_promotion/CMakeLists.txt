# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(global_promotion)

add_subdirectory(subdir_with_local_qt)
add_subdirectory(subdir_with_global_qt)

set(file_path "${CMAKE_CURRENT_BINARY_DIR}/main.cpp")
file(GENERATE OUTPUT "${file_path}" CONTENT "int main() { return 0; }")
add_executable(exe main.cpp)

# The Qt targets found in the 2nd child directory scope should be available in this scope.
target_link_libraries(exe PRIVATE lib_global_qt)
