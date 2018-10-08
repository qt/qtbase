TARGET = qeglfs-kms-egldevice-integration

QT += core-private gui-private eglfsdeviceintegration-private eglfs_kms_support-private kms_support-private edid_support-private

INCLUDEPATH += $$PWD/../../api $$PWD/../eglfs_kms_support

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

QMAKE_USE += drm
CONFIG += egl

SOURCES += $$PWD/qeglfskmsegldevicemain.cpp \
           $$PWD/qeglfskmsegldeviceintegration.cpp \
    qeglfskmsegldevice.cpp \
    qeglfskmsegldevicescreen.cpp

HEADERS += $$PWD/qeglfskmsegldeviceintegration.h \
    qeglfskmsegldevice.h \
    qeglfskmsegldevicescreen.h

OTHER_FILES += $$PWD/eglfs_kms_egldevice.json

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSKmsEglDeviceIntegrationPlugin
load(qt_plugin)
