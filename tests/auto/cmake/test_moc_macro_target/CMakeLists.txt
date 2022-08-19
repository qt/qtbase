# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(test_moc_macro_target)

find_package(Qt6Core REQUIRED)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

qt_generate_moc(main_gen_test.cpp
  "${CMAKE_CURRENT_BINARY_DIR}/main_gen_test.moc"
  TARGET QtGenerateMacroTest
)
add_executable(QtGenerateMacroTest main_gen_test.cpp "${CMAKE_CURRENT_BINARY_DIR}/main_gen_test.moc")
target_include_directories(QtGenerateMacroTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/interface")
target_link_libraries(QtGenerateMacroTest PRIVATE Qt6::Core)

qt_wrap_cpp(moc_file mywrapobject.h
  TARGET QtWrapMacroTest
)
add_executable(QtWrapMacroTest main_wrap_test.cpp ${moc_file})
target_include_directories(QtWrapMacroTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/interface")
target_link_libraries(QtWrapMacroTest PRIVATE Qt::Core)
