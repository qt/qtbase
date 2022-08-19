# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from socketengine.pro.

#####################################################################
## tst_socketengine Binary:
#####################################################################

# special case begin
if (NOT QT_FEATURE_private_tests)
    return()
endif()
# special case end

qt_internal_add_manual_test(tst_socketengine
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
        Qt::Test
)

#### Keys ignored in scope 1:.:.:socketengine.pro:<TRUE>:
# TEMPLATE = "app"
