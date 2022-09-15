# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
if (NOT CMAKE_PROJECT_NAME STREQUAL "QtBase" AND NOT CMAKE_PROJECT_NAME STREQUAL "Qt")
    include(.cmake.conf)
    project(QSQLiteDriverPlugins
        VERSION "${QT_REPO_MODULE_VERSION}"
        DESCRIPTION "Qt6 SQL driver plugins"
        HOMEPAGE_URL "https://qt.io/"
        LANGUAGES CXX C ASM
    )
    find_package(Qt6 ${PROJECT_VERSION} CONFIG REQUIRED COMPONENTS
        BuildInternals
        Core
        Sql
     )
    qt_prepare_standalone_project()
else()
    qt_internal_upgrade_cmake_policies()
endif()

# Currently handled completely manually.

# TODO sqldrivers_standalone {
#     _QMAKE_CACHE_ = $$shadowed($$SQLDRV_SRC_TREE)/.qmake.conf
#     load(qt_configure)
# }

qt_feature_module_begin(
    NO_MODULE
    PUBLIC_FILE "qtsqldrivers-config.h"
    PRIVATE_FILE "qtsqldrivers-config_p.h"
)
include(configure.cmake)
qt_feature_module_end(NO_MODULE)


if(QT_FEATURE_sql_psql AND QT_FEATURE_regularexpression)
    add_subdirectory(psql)
endif()

if(QT_FEATURE_sql_mysql)
     add_subdirectory(mysql)
endif()

if(QT_FEATURE_sql_odbc)
    add_subdirectory(odbc)
endif()

if(QT_FEATURE_sql_oci)
    add_subdirectory(oci)
endif()

if(QT_FEATURE_sql_db2)
    add_subdirectory(db2)
endif()

if(QT_FEATURE_sql_sqlite)
    add_subdirectory(sqlite)
endif()

if(QT_FEATURE_sql_ibase)
    add_subdirectory(ibase)
endif()

if(QT_FEATURE_sql_mimer)
    add_subdirectory(mimer)
endif()

if(NOT CMAKE_PROJECT_NAME STREQUAL "QtBase" AND NOT CMAKE_PROJECT_NAME STREQUAL "Qt")
    qt_print_feature_summary()
endif()
