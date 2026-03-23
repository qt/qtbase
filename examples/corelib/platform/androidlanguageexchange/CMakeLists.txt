# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(androidlanguageexchange LANGUAGES CXX)

if(NOT ANDROID)
    message(FATAL_ERROR "Example only works on Android")
endif()

find_package(Qt6 REQUIRED COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(androidlanguageexchange
    main.cpp
    dataexchanger.h dataexchanger.cpp
    otherlanguagehandler.h otherlanguagehandler.cpp
)

target_link_libraries(androidlanguageexchange PRIVATE
    Qt6::Widgets
)

set_target_properties(androidlanguageexchange PROPERTIES
    QT_ANDROID_APP_NAME "Qt Language Exchange"
    QT_ANDROID_APP_ICON "@drawable/icon"
    QT_ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android"
)

install(TARGETS androidlanguageexchange
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

