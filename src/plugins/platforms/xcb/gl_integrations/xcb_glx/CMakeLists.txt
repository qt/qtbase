# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QXcbGlxIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QXcbGlxIntegrationPlugin
    OUTPUT_NAME qxcb-glx-integration
    PLUGIN_TYPE xcbglintegrations
    SOURCES
        qglxintegration.cpp qglxintegration.h
        qxcbglxintegration.cpp qxcbglxintegration.h
        qxcbglxmain.cpp
        qxcbglxnativeinterfacehandler.cpp qxcbglxnativeinterfacehandler.h
        qxcbglxwindow.cpp qxcbglxwindow.h
    DEFINES
        QT_NO_FOREACH
    INCLUDE_DIRECTORIES
        ..
        ../..
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::XcbQpaPrivate
    NO_UNITY_BUILD # X11 define clashes
)

## Scopes:
#####################################################################

qt_internal_extend_target(QXcbGlxIntegrationPlugin CONDITION QT_FEATURE_xcb_glx
    LIBRARIES
        XCB::GLX
)

qt_internal_extend_target(QXcbGlxIntegrationPlugin
    CONDITION QT_FEATURE_dlopen AND QT_BUILD_SHARED_LIBS
    LIBRARIES
        ${CMAKE_DL_LIBS}
)
