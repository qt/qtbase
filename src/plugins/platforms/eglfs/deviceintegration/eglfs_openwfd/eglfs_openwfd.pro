TARGET = qeglfs-openwfd-integration

QT += core-private gui-private eglfsdeviceintegration-private

INCLUDEPATH += $$PWD/../../api
CONFIG += egl

SOURCES += $$PWD/qeglfsopenwfdmain.cpp \
           $$PWD/qeglfsopenwfdintegration.cpp

HEADERS += $$PWD/qeglfsopenwfdintegration.h

OTHER_FILES += $$PWD/eglfs_openwfd.json

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSOpenWFDIntegrationPlugin
load(qt_plugin)
