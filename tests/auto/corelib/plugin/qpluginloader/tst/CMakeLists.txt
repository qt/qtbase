# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpluginloader Test:
#####################################################################

# Collect test data
list(APPEND test_data "../elftest")
list(APPEND test_data "../machtest")

qt_internal_add_test(tst_qpluginloader
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    SOURCES
        ../fakeplugin.cpp
        ../theplugin/plugininterface.h
        ../tst_qpluginloader.cpp
    LIBRARIES
        Qt::CorePrivate
        staticplugin
    TESTDATA ${test_data}
)

add_dependencies(tst_qpluginloader tst_qpluginloaderlib staticplugin theplugin)
if (UNIX)
    if(NOT APPLE)
        add_dependencies(tst_qpluginloader theoldplugin)
    endif()
    if (NOT ANDROID AND NOT APPLE)
        add_dependencies(tst_qpluginloader almostplugin)
    endif()
endif()

if(ANDROID)
    add_compile_definitions(ANDROID_ARCH="${CMAKE_ANDROID_ARCH_ABI}")
    set(plugins
        theplugin
        theoldplugin
        tst_qpluginloaderlib
    )
    set(extra_libs)
    foreach(plugin IN LISTS plugins)
        list(APPEND extra_libs
            "${CMAKE_CURRENT_BINARY_DIR}/../bin/lib${plugin}_${CMAKE_ANDROID_ARCH_ABI}.so")
    endforeach()
    set_target_properties(tst_qpluginloader PROPERTIES
        QT_ANDROID_EXTRA_LIBS "${extra_libs}"
    )
endif()
