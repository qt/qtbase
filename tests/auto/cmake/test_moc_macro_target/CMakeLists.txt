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
list(APPEND CMAKE_AUTOMOC_MACRO_NAMES MySpecialMacro)
add_executable(QtGenerateMacroTest main_gen_test.cpp "${CMAKE_CURRENT_BINARY_DIR}/main_gen_test.moc")
get_target_property(current_macros QtGenerateMacroTest AUTOMOC_MACRO_NAMES)

if(NOT "${CMAKE_AUTOMOC_MACRO_NAMES}" STREQUAL "${current_macros}")
    message(FATAL_ERROR "Expected ${CMAKE_AUTOMOC_MACRO_NAMES} but received ${current_macros}")
endif()

target_include_directories(QtGenerateMacroTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/interface")
target_link_libraries(QtGenerateMacroTest PRIVATE Qt6::Core)

qt_wrap_cpp(moc_file mywrapobject.h
  TARGET QtWrapMacroTest
)
add_executable(QtWrapMacroTest main_wrap_test.cpp ${moc_file})
target_include_directories(QtWrapMacroTest PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/interface")
target_link_libraries(QtWrapMacroTest PRIVATE Qt::Core)

target_compile_definitions(QtWrapMacroTest PRIVATE "$<$<BOOL:TRUE>:MY_OPTION>"
                                                   "$<$<BOOL:TRUE>:DEFINE_CMDLINE_SIGNAL=void cmdlineSignal(const QMap<int$<COMMA> int$<ANGLE-R> &i)>"
                                                    "DEFINE_CMDLINE_SIGNAL=void cmdlineSignal(const QMap<int, int> &i)")

set(parameters_file_base "${CMAKE_CURRENT_BINARY_DIR}/moc_mywrapobject.cpp_parameters")
# check if generator is multi-config
if(CMAKE_CONFIGURATION_TYPES)
  set(parameters_file "${parameters_file_base}_$<CONFIG>")
else()
  if(NOT CMAKE_BUILD_TYPE STREQUAL "")
    set(parameters_file "${parameters_file_base}_${CMAKE_BUILD_TYPE}")
  else()
    set(parameters_file "${parameters_file_base}")
  endif()
endif()

add_custom_command(TARGET QtWrapMacroTest
  POST_BUILD
  COMMAND ${CMAKE_COMMAND} "-DPARAMETERS_FILE_PATH=${parameters_file}" -P check_moc_parameters.cmake
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
