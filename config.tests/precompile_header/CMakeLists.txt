# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(precompile_header LANGUAGES CXX)

add_executable(precompile_header main.cpp)
target_precompile_headers(precompile_header PRIVATE header.h)
