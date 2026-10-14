# Copyright (C) 2026 Intel Corporation.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_cmake_library(tst_qfactoryloader_futureplugin
    MODULE
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../future/bin"
    SOURCES
        futureplugin.cpp futureplugin.h
    LIBRARIES
        Qt::Core
)

if(ANDROID)
    # On Android the plugins must match the following mask:
    # "libplugins_{suffix}_*.so"
    # and the testcase uses "bin" as a suffix
    set_target_properties(tst_qfactoryloader_futureplugin PROPERTIES
        OUTPUT_NAME "plugins_bin_tst_qfactoryloader_futureplugin")
endif()

add_dependencies(tst_qfactoryloader tst_qfactoryloader_futureplugin)

qt_autogen_tools_initial_setup(tst_qfactoryloader_futureplugin)
