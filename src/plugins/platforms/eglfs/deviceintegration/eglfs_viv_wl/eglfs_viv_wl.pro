TARGET = qeglfs-viv-wl-integration

QT += core-private gui-private eglfsdeviceintegration-private

INCLUDEPATH += $$PWD/../../api
CONFIG += egl
DEFINES += LINUX=1 EGL_API_FB=1
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

SOURCES += $$PWD/qeglfsvivwlmain.cpp \
           $$PWD/qeglfsvivwlintegration.cpp

HEADERS += $$PWD/qeglfsvivwlintegration.h

OTHER_FILES += $$PWD/eglfs_viv_wl.json

CONFIG += link_pkgconfig
PKGCONFIG_PRIVATE += wayland-server

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSVivWaylandIntegrationPlugin
load(qt_plugin)
