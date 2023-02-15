# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## plugin1 Generic Library:
#####################################################################

qt_internal_add_cmake_library(tst_qfactoryloader_plugin1
    MODULE
    INSTALL_DIRECTORY "${INSTALL_TESTSDIR}/tst_qfactoryloader/bin"
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../bin"
    SOURCES
        plugin1.cpp plugin1.h
    LIBRARIES
        Qt::Core
)

if(ANDROID)
    # On Android the plugins must match the following mask:
    # "libplugins_{suffix}_*.so"
    # and the testcase uses "bin" as a suffix
    set_target_properties(tst_qfactoryloader_plugin1 PROPERTIES
        OUTPUT_NAME "plugins_bin_tst_qfactoryloader_plugin1")
endif()

qt_internal_extend_target(tst_qfactoryloader_plugin1 CONDITION NOT QT_FEATURE_library
    DEFINES
        QT_STATICPLUGIN
)

qt_autogen_tools_initial_setup(tst_qfactoryloader_plugin1)
