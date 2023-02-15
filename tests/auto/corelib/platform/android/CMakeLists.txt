# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_android Test:
#####################################################################

qt_internal_add_test(tst_android
    SOURCES
        tst_android.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

if(ANDROID)
    set_property(TARGET tst_android APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
        ${CMAKE_CURRENT_SOURCE_DIR}/testdata
    )
endif()
