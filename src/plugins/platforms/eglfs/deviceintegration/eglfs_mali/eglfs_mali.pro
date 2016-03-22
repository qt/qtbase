TARGET = qeglfs-mali-integration

QT += core-private gui-private platformsupport-private eglfs_device_lib-private

# Avoid X11 header collision
DEFINES += MESA_EGL_NO_X11_HEADERS

INCLUDEPATH += $$PWD/../..
CONFIG += egl
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

SOURCES += $$PWD/qeglfsmalimain.cpp \
           $$PWD/qeglfsmaliintegration.cpp

HEADERS += $$PWD/qeglfsmaliintegration.h

OTHER_FILES += $$PWD/eglfs_mali.json

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSMaliIntegrationPlugin
load(qt_plugin)
