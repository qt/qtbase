TARGET = qeglfs-kms-vsp2-integration

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSKmsVsp2IntegrationPlugin
load(qt_plugin)

QT += core-private gui-private eglfsdeviceintegration-private eglfs_kms_support-private kms_support-private edid_support-private

INCLUDEPATH += $$PWD/../../api $$PWD/../eglfs_kms_support

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

QMAKE_USE += gbm drm v4l2
CONFIG += egl

SOURCES += $$PWD/qeglfskmsvsp2main.cpp \
           $$PWD/qeglfskmsvsp2integration.cpp \
           $$PWD/qeglfskmsvsp2device.cpp \
           $$PWD/qeglfskmsvsp2screen.cpp \
           $$PWD/qlinuxmediadevice.cpp \
           $$PWD/qvsp2blendingdevice.cpp

HEADERS += $$PWD/qeglfskmsvsp2integration.h \
           $$PWD/qeglfskmsvsp2device.h \
           $$PWD/qeglfskmsvsp2screen.h \
           $$PWD/qlinuxmediadevice.h \
           $$PWD/qvsp2blendingdevice.h

OTHER_FILES += $$PWD/eglfs_kms.json
