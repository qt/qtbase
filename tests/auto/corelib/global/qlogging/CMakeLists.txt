# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qlogging LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_executable(qlogging_helper
    NO_INSTALL
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES qlogging_helper.cpp ../../../../shared/disablecoredumps.cpp
    DEFINES QT_MESSAGELOGCONTEXT
    LIBRARIES Qt::Core)

qt_internal_add_executable(qlogging_race_helper
    NO_INSTALL
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    SOURCES qlogging_race_helper.cpp ../../../../shared/disablecoredumps.cpp
    LIBRARIES Qt::Core
)

# Fixes required for the backtrace stack to be correct
if (${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU" AND NOT MINGW)
    target_link_options(qlogging_helper PRIVATE -rdynamic)
endif()
set_target_properties(qlogging_helper PROPERTIES CXX_VISIBILITY_PRESET default)

qt_internal_add_test(tst_qlogging SOURCES tst_qlogging.cpp
    DEFINES
        QT_MESSAGELOGCONTEXT
)

add_dependencies(tst_qlogging qlogging_helper qlogging_race_helper)

qt_internal_add_test(tst_qmessagelogger SOURCES tst_qmessagelogger.cpp
    DEFINES
        QT_MESSAGELOGCONTEXT
)
