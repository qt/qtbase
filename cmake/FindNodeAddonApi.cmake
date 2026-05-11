# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(TARGET NodeAddonApi::NodeAddonApi)
    set(NodeAddonApi_FOUND ON)
    return()
endif()

find_path(NodeAddonApi_INCLUDE_DIR
    NAMES napi.h
    HINTS
        ${NODE_ADDON_API_ROOT}
        $ENV{NODE_ADDON_API_ROOT}
    PATH_SUFFIXES node-addon-api
)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(NodeAddonApi DEFAULT_MSG NodeAddonApi_INCLUDE_DIR)

if(NodeAddonApi_FOUND AND NOT TARGET NodeAddonApi::NodeAddonApi)
    add_library(NodeAddonApi::NodeAddonApi INTERFACE IMPORTED)
    target_include_directories(NodeAddonApi::NodeAddonApi INTERFACE
        ${NodeAddonApi_INCLUDE_DIR})

    set_target_properties(NodeAddonApi::NodeAddonApi PROPERTIES
        _qt_is_nolink_target TRUE)
endif()

