# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QWindowsVistaStylePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QModernWindowsStylePlugin
    OUTPUT_NAME qmodernwindowsstyle
    PLUGIN_TYPE styles
    SOURCES
        main.cpp
        qwindowsvistastyle.cpp qwindowsvistastyle_p.h
        qwindows11style.cpp qwindows11style_p.h
        qwindowsvistastyle_p_p.h
        qwindowsvistaanimation.cpp qwindowsvistaanimation_p.h
        qwindowsthemedata.cpp qwindowsthemedata_p.h

    LIBRARIES
        gdi32
        user32
        uxtheme
        Qt::Core
        Qt::Gui
        Qt::GuiPrivate
        Qt::WidgetsPrivate
)
