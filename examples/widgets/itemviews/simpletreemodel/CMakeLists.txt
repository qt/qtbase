# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(simpletreemodel LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Test Widgets)

qt_standard_project_setup()

qt_add_executable(simpletreemodel
    main.cpp
    treeitem.cpp treeitem.h
    treemodel.cpp treemodel.h
)

set_target_properties(simpletreemodel PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_link_libraries(simpletreemodel PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

# Resources:
set(simpletreemodel_resource_files
    "default.txt"
)

qt_add_resources(simpletreemodel "simpletreemodel"
    PREFIX
        "/"
    FILES
        ${simpletreemodel_resource_files}
)

install(TARGETS simpletreemodel
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET simpletreemodel
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})

#! [1]
# Unit Test

include(CTest)

qt_add_executable(simpletreemodel_tester
    test.cpp
    treeitem.cpp treeitem.h
    treemodel.cpp treemodel.h)

target_link_libraries(simpletreemodel_tester PRIVATE
    Qt6::Core
    Qt6::Test
)

if(ANDROID)
    target_link_libraries(simpletreemodel_tester PRIVATE
        Qt6::Gui
    )
endif()

qt_add_resources(simpletreemodel_tester "simpletreemodel_tester"
    PREFIX
        "/"
    FILES
        ${simpletreemodel_resource_files}
)

add_test(NAME simpletreemodel_tester
         COMMAND simpletreemodel_tester)
#! [1]
