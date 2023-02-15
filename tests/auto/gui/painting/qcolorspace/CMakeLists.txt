# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qcolorspace Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    resources/*
)

foreach(file IN LISTS test_data)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

qt_internal_add_test(tst_qcolorspace
    SOURCES
        tst_qcolorspace.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
    BUILTIN_TESTDATA
)
