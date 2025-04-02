# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#! [qt_generate_deploy_script_example]
cmake_minimum_required(VERSION 3.16...3.22)
project(MyThings)

find_package(Qt6 REQUIRED COMPONENTS Core)
qt_standard_project_setup()

qt_add_executable(MyApp main.cpp)

install(TARGETS MyApp
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_script(
    TARGET MyApp
    OUTPUT_SCRIPT deploy_script
    CONTENT "
qt_deploy_runtime_dependencies(
    EXECUTABLE $<TARGET_FILE:MyApp>
)
")
install(SCRIPT ${deploy_script})
#! [qt_generate_deploy_script_example]
