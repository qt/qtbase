# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(rangemodel VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 COMPONENTS
    Core Gui Widgets
    REQUIRED
)

find_package(Qt6 OPTIONAL_COMPONENTS
    Quick
    QuickWidgets
)

if (TARGET Qt6::Quick)
    qt_policy(SET QTP0001 NEW)
endif()

qt_standard_project_setup()

qt_add_executable(rangemodel
    MANUAL_FINALIZATION
    main.cpp
)

target_compile_options(rangemodel PRIVATE -fsanitize=address)
target_link_options(rangemodel PRIVATE -fsanitize=address)

target_link_libraries(rangemodel PRIVATE
    Qt6::Widgets
)

if (TARGET Qt6::Quick)
    target_link_libraries(rangemodel PRIVATE
       Qt6::Quick
       Qt6::QuickWidgets
    )
endif()

set_property(TARGET rangemodel APPEND PROPERTY
             QT_ANDROID_PACKAGE_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
)

if ("${CMAKE_CXX_COMPILE_FEATURES}" MATCHES "cxx_std_23")
    set_property(TARGET rangemodel PROPERTY CXX_STANDARD 23)
endif()

qt_finalize_executable(rangemodel)

if (TARGET Qt6::Quick)
    qt_add_qml_module(rangemodel
        URI Main
        VERSION 1.0
        QML_FILES "Main.qml"
        NO_RESOURCE_TARGET_PATH
    )
endif()

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
