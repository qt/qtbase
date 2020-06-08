TEMPLATE = app
TARGET = qopenglcontext

QT += gui-private egl_support-private opengl

HEADERS += $$PWD/qopenglcontextwindow.h

SOURCES += $$PWD/main.cpp \
           $$PWD/qopenglcontextwindow.cpp
