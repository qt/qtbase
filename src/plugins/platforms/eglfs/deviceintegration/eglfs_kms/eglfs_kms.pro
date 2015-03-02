TARGET = qeglfs-kms-integration

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSKmsIntegrationPlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private eglfs_device_lib-private

INCLUDEPATH += $$PWD/../..

# Avoid X11 header collision
DEFINES += MESA_EGL_NO_X11_HEADERS

CONFIG += link_pkgconfig
PKGCONFIG += libdrm gbm

CONFIG += egl
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

SOURCES += $$PWD/qeglfskmsmain.cpp \
           $$PWD/qeglfskmsintegration.cpp \
           $$PWD/qeglfskmsdevice.cpp \
           $$PWD/qeglfskmsscreen.cpp \
           $$PWD/qeglfskmscursor.cpp

HEADERS += $$PWD/qeglfskmsintegration.h \
           $$PWD/qeglfskmsdevice.h \
           $$PWD/qeglfskmsscreen.h \
           $$PWD/qeglfskmscursor.h

OTHER_FILES += $$PWD/eglfs_kms.json
