TARGET     = QtOpenGLExtensions
CONFIG += static

qtConfig(opengl): CONFIG += opengl
qtConfig(opengles2): CONFIG += opengles2

DEFINES += QT_NO_CAST_FROM_ASCII QT_NO_FOREACH

PRECOMPILED_HEADER =

HEADERS = qopenglextensions.h

SOURCES = qopenglextensions.cpp

load(qt_module)
