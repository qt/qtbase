TARGET = qeglfs-viv-integration

QT += core-private gui-private eglfsdeviceintegration-private

INCLUDEPATH += $$PWD/../../api
CONFIG += egl
DEFINES += LINUX=1 EGL_API_FB=1

SOURCES += $$PWD/qeglfsvivmain.cpp \
           $$PWD/qeglfsvivintegration.cpp

HEADERS += $$PWD/qeglfsvivintegration.h

OTHER_FILES += $$PWD/eglfs_viv.json

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSVivIntegrationPlugin
load(qt_plugin)
