TARGET = qeglfs-x11-integration

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSX11IntegrationPlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private eglfs_device_lib-private

# Avoid X11 header collision
DEFINES += MESA_EGL_NO_X11_HEADERS

INCLUDEPATH += $$PWD/../..

CONFIG += egl
LIBS += -lX11 -lX11-xcb -lxcb
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

SOURCES += $$PWD/qeglfsx11main.cpp \
           $$PWD/qeglfsx11integration.cpp

HEADERS += $$PWD/qeglfsx11integration.h

OTHER_FILES += $$PWD/eglfs_x11.json
