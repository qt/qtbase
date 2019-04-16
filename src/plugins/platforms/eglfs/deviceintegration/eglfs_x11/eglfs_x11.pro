TARGET = qeglfs-x11-integration

QT += core-private gui-private eglfsdeviceintegration-private

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

INCLUDEPATH += $$PWD/../../api

CONFIG += egl
QMAKE_USE += xcb_xlib xcb xlib

SOURCES += $$PWD/qeglfsx11main.cpp \
           $$PWD/qeglfsx11integration.cpp

HEADERS += $$PWD/qeglfsx11integration.h

OTHER_FILES += $$PWD/eglfs_x11.json

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSX11IntegrationPlugin
load(qt_plugin)
