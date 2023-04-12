# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(objcopytest LANGUAGES CXX)
add_executable(objcopytest main.cpp)
add_custom_command(
    TARGET objcopytest
    POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} --only-keep-debug $<TARGET_FILE:objcopytest> objcopytest.debug
    COMMAND ${CMAKE_OBJCOPY} --strip-debug $<TARGET_FILE:objcopytest>
    COMMAND ${CMAKE_OBJCOPY} --add-gnu-debuglink=objcopytest.debug $<TARGET_FILE:objcopytest>
    VERBATIM)
