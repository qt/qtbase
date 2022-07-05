# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from openglwidgets.pro.

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
