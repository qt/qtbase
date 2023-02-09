# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## OpenGLWidgets Module:
#####################################################################

qt_internal_add_module(OpenGLWidgets
    SOURCES
        qopenglwidget.cpp qopenglwidget.h
        qtopenglwidgetsglobal.h
    DEFINES
        QT_NO_FOREACH
        QT_NO_USING_NAMESPACE
    LIBRARIES
        Qt::OpenGLPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::OpenGL
        Qt::Widgets
    GENERATE_CPP_EXPORTS
)
