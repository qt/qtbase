# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (TARGET WrapCPDB::WrapCPDB)
    set(WrapCPDB_FOUND ON)
    return()
endif()

find_package(PkgConfig QUIET)
if (PKG_CONFIG_FOUND)
    pkg_check_modules(CPDB QUIET cpdb-frontend)
    if (CPDB_FOUND)
        add_library(WrapCPDB::WrapCPDB INTERFACE IMPORTED)
        target_compile_definitions(WrapCPDB::WrapCPDB INTERFACE -DQT_NO_KEYWORDS)
        set_target_properties(WrapCPDB::WrapCPDB PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${CPDB_INCLUDE_DIRS}")
        set_target_properties(WrapCPDB::WrapCPDB PROPERTIES
            INTERFACE_LINK_LIBRARIES "${CPDB_LIBRARIES}")
    endif()
endif()

find_package_handle_standard_args(WrapCPDB
                                  REQUIRED_VARS CPDB_INCLUDE_DIRS CPDB_LIBRARIES
                                  VERSION_VAR CPDB_VERSION)

