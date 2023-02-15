# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qpathclipper Test:
#####################################################################

qt_internal_add_test(tst_qpathclipper
    SOURCES
        paths.cpp paths.h
        tst_qpathclipper.cpp
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qpathclipper CONDITION UNIX AND NOT APPLE AND NOT HAIKU AND NOT INTEGRITY
    LIBRARIES
        m
)
