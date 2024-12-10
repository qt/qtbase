# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_manual_windowflags Binary:
#####################################################################

qt_internal_add_manual_test(tst_manual_windowflags
    GUI
    SOURCES
        controllerwindow.cpp controllerwindow.h
        controls.cpp controls.h
        main.cpp
        previewwindow.cpp previewwindow.h
    NO_PCH_SOURCES
        controls.cpp # undef QT_NO_FOREACH
    LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
)
