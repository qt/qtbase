# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## embeddedintoforeignwindow Binary:
#####################################################################

qt_internal_add_manual_test(embeddedintoforeignwindow
    SOURCES
        ../diaglib/eventfilter.cpp ../diaglib/eventfilter.h
        ../diaglib/nativewindowdump.h
        ../diaglib/qwindowdump.cpp ../diaglib/qwindowdump.h
        ../diaglib/textdump.cpp ../diaglib/textdump.h
        itemwindow.cpp itemwindow.h
        main.cpp
    DEFINES
        QT_DIAG_LIB
    INCLUDE_DIRECTORIES
        ../diaglib
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(embeddedintoforeignwindow CONDITION WIN32
    SOURCES
        ../diaglib/nativewindowdump_win.cpp
    LIBRARIES
        user32
)

qt_internal_extend_target(embeddedintoforeignwindow CONDITION UNIX
    SOURCES
        ../diaglib/nativewindowdump.cpp
)

qt_internal_extend_target(embeddedintoforeignwindow CONDITION QT_FEATURE_widgets
    SOURCES
        ../diaglib/debugproxystyle.cpp ../diaglib/debugproxystyle.h
        ../diaglib/logwidget.cpp ../diaglib/logwidget.h
        ../diaglib/qwidgetdump.cpp ../diaglib/qwidgetdump.h
    LIBRARIES
        Qt::WidgetsPrivate
)

qt_internal_extend_target(embeddedintoforeignwindow CONDITION QT_FEATURE_opengl
    SOURCES
        ../diaglib/glinfo.cpp ../diaglib/glinfo.h
    LIBRARIES
        Qt::OpenGL
        Qt::OpenGLWidgets
)
