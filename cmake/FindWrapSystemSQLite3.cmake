# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET WrapSystemSQLite3::WrapSystemSQLite3)
    set(WrapSystemSQLite3_FOUND ON)
    return()
endif()
set(WrapSystemSQLite3_FOUND OFF)

find_package(SQLite3 ${${CMAKE_FIND_PACKAGE_NAME}_FIND_VERSION} QUIET)

if(SQLite3_FOUND)
    set(WrapSystemSQLite3_FOUND ON)

    add_library(WrapSystemSQLite3::WrapSystemSQLite3 INTERFACE IMPORTED)
    if(TARGET SQLite3::SQLite3)
        target_link_libraries(WrapSystemSQLite3::WrapSystemSQLite3 INTERFACE SQLite3::SQLite3)
    else()
        # Old target name is deprecated since CMake 4.3.
        target_link_libraries(WrapSystemSQLite3::WrapSystemSQLite3 INTERFACE SQLite::SQLite3)
    endif()
endif()

if(SQLite3_VERSION)
    set(WrapSystemSQLite3_VERSION "${SQLite3_VERSION}")
endif()

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(WrapSystemSQLite3
                                  REQUIRED_VARS SQLite3_LIBRARIES SQLite3_INCLUDE_DIRS
                                  VERSION_VAR WrapSystemSQLite3_VERSION)
