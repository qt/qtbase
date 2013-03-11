TARGET     = QtOpenGLExtensions
CONFIG += static

contains(QT_CONFIG, opengl):CONFIG += opengl
contains(QT_CONFIG, opengles1):CONFIG += opengles1
contains(QT_CONFIG, opengles2):CONFIG += opengles2

load(qt_module)

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../corelib/global/qt_pch.h

HEADERS = qopenglextensions.h

SOURCES = qopenglextensions.cpp
