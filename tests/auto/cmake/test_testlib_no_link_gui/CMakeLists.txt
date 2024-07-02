# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)

project(no_link_gui)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

if (EXISTS "${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
    include("${CMAKE_CURRENT_LIST_DIR}/FindPackageHints.cmake")
endif()

find_package(Qt6Gui REQUIRED HINTS ${Qt6Tests_PREFIX_PATH})
find_package(Qt6Test REQUIRED HINTS ${Qt6Tests_PREFIX_PATH})

include_directories(
    ${Qt6Gui_INCLUDE_DIRS}
    ${Qt6Test_INCLUDE_DIRS}
)

add_compile_definitions(
    ${Qt6Gui_DEFINITIONS}
    ${Qt6Test_DEFINITIONS}
)

set(main_file "main.cpp")
set(moc_file "${CMAKE_CURRENT_BINARY_DIR}/main.moc")

qt_generate_moc("${main_file}" "${moc_file}")

# The core_test is expected to fail to build because
# QT_GUI_LIB is defined, which affects the contents of
# Qt Test and the definition of QTEST_MAIN.
# If running this test manually (ctest -V -R no_link_gui from
# the tests/auto/cmake/build directory), the core_test is
# expected to fail to link because of missing symbols from Qt GUI.
# The gui_test is expected to build successfully  (though it may
# be necessary to comment out the core_test and re-run cmake)
add_executable(core_test "${main_file}" "${moc_file}")
target_link_libraries(core_test
    ${Qt6Core_LIBRARIES}
    ${Qt6Test_LIBRARIES}
)

add_executable(gui_test "${main_file}" "${moc_file}")
target_link_libraries(gui_test
    ${Qt6Gui_LIBRARIES}
    ${Qt6Test_LIBRARIES}
)
