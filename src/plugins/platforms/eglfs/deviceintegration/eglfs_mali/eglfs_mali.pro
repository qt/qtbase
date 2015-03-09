TARGET = qeglfs-mali-integration

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSMaliIntegrationPlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private eglfs_device_lib-private

INCLUDEPATH += $$PWD/../..
CONFIG += egl
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

SOURCES += $$PWD/qeglfsmalimain.cpp \
           $$PWD/qeglfsmaliintegration.cpp

HEADERS += $$PWD/qeglfsmaliintegration.h

OTHER_FILES += $$PWD/eglfs_mali.json
