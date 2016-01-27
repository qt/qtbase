TARGET = qeglfs-kms-egldevice-integration

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSKmsEglDeviceIntegrationPlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private eglfs_device_lib-private

INCLUDEPATH += $$PWD/../..

DEFINES += MESA_EGL_NO_X11_HEADERS

CONFIG += egl
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

SOURCES += $$PWD/qeglfskmsegldevicemain.cpp \
           $$PWD/qeglfskmsegldeviceintegration.cpp

HEADERS += $$PWD/qeglfskmsegldeviceintegration.h

OTHER_FILES += $$PWD/eglfs_kms_egldevice.json

LIBS += -ldrm
