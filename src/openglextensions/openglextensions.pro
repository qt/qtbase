TARGET     = QtOpenGLExtensions
QT = core
CONFIG += static

load(qt_module)

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER = ../corelib/global/qt_pch.h

HEADERS = qopenglextensions.h

SOURCES = qopenglextensions.cpp
