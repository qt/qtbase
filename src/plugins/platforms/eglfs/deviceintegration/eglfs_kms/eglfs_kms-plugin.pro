TARGET = qeglfs-kms-integration

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSKmsGbmIntegrationPlugin
load(qt_plugin)

QT += core-private gui-private eglfsdeviceintegration-private eglfs_kms_support-private kms_support-private eglfs_kms_gbm_support-private

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

QMAKE_USE += gbm drm
CONFIG += egl

SOURCES += $$PWD/qeglfskmsgbmmain.cpp

OTHER_FILES += $$PWD/eglfs_kms.json
