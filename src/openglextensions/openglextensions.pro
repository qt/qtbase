TARGET     = QtOpenGLExtensions
CONFIG += static

contains(QT_CONFIG, opengl):CONFIG += opengl
contains(QT_CONFIG, opengles2):CONFIG += opengles2

DEFINES += QT_NO_CAST_FROM_ASCII
PRECOMPILED_HEADER =

HEADERS = qopenglextensions.h

SOURCES = qopenglextensions.cpp

load(qt_module)
