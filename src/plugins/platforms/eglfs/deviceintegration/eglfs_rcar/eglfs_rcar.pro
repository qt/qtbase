TARGET = qeglfs-rcar-integration

QT += core-private gui-private eglfsdeviceintegration-private

INCLUDEPATH += $$PWD/../../api
CONFIG += egl
DEFINES += LINUX=1 EGL_API_FB=1
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

SOURCES += $$PWD/qeglfsrcarmain.cpp \
           $$PWD/qeglfsrcarintegration.cpp

HEADERS += $$PWD/qeglfsrcarintegration.h

OTHER_FILES += $$PWD/eglfs_rcar.json

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSRcarIntegrationPlugin
load(qt_plugin)
