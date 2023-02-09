# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## FbSupportPrivate Module:
#####################################################################

qt_internal_add_module(FbSupportPrivate
    CONFIG_MODULE_NAME fb_support
    STATIC
    INTERNAL_MODULE
    SOURCES
        qfbbackingstore.cpp qfbbackingstore_p.h
        qfbcursor.cpp qfbcursor_p.h
        qfbscreen.cpp qfbscreen_p.h
        qfbvthandler.cpp qfbvthandler_p.h
        qfbwindow.cpp qfbwindow_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PRECOMPILED_HEADER
        "../../corelib/global/qt_pch.h"
)
