# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_find_package(DirectFB)
qt_find_package(EGL)

#####################################################################
## QDirectFbIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QDirectFbIntegrationPlugin
    OUTPUT_NAME qdirectfb
    PLUGIN_TYPE platforms
    SOURCES
        main.cpp
        qdirectfbbackingstore.cpp qdirectfbbackingstore.h
        qdirectfbblitter.cpp qdirectfbblitter.h
        qdirectfbconvenience.cpp qdirectfbconvenience.h
        qdirectfbcursor.cpp qdirectfbcursor.h
        qdirectfbeglhooks.h
        qdirectfbinput.cpp qdirectfbinput.h
        qdirectfbintegration.cpp qdirectfbintegration.h
        qdirectfbscreen.cpp qdirectfbscreen.h
        qdirectfbwindow.cpp qdirectfbwindow.h
    LIBRARIES
        PkgConfig::DirectFB
        EGL::EGL
        Qt::Core
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(QDirectFbIntegrationPlugin CONDITION QT_CONFIG___contains___directfb_egl
    SOURCES
        qdirectfb_egl.cpp qdirectfb_egl.h
    DEFINES
        DIRECTFB_GL_EGL
)

qt_internal_extend_target(QDirectFbIntegrationPlugin CONDITION NOT DIRECTFB_PLATFORM_HOOKS_SOURCES_ISEMPTY
    DEFINES
        DIRECTFB_PLATFORM_HOOKS
)

qt_internal_extend_target(QDirectFbIntegrationPlugin CONDITION DIRECTFB_PLATFORM_HOOKS_SOURCES_ISEMPTY
    SOURCES
        qdirectfbeglhooks_stub.cpp
)
