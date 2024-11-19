# Copyright (C) 2024 Intel Corporation.
# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required(VERSION 3.16)
project(cxa_thread_atexit LANGUAGES C)
add_executable(cxa_thread_atexit main.c)
target_compile_definitions(cxa_thread_atexit PUBLIC TEST_FUNC=${TEST_FUNC})
