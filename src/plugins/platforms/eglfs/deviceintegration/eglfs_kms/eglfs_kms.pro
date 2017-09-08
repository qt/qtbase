TARGET = qeglfs-kms-integration

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSKmsGbmIntegrationPlugin
load(qt_plugin)

QT += core-private gui-private eglfsdeviceintegration-private eglfs_kms_support-private kms_support-private edid_support-private

INCLUDEPATH += $$PWD/../../api $$PWD/../eglfs_kms_support

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

QMAKE_USE += gbm drm
CONFIG += egl
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

SOURCES += $$PWD/qeglfskmsgbmmain.cpp \
           $$PWD/qeglfskmsgbmintegration.cpp \
           $$PWD/qeglfskmsgbmdevice.cpp \
           $$PWD/qeglfskmsgbmscreen.cpp \
           $$PWD/qeglfskmsgbmcursor.cpp \
           $$PWD/qeglfskmsgbmwindow.cpp

HEADERS += $$PWD/qeglfskmsgbmintegration.h \
           $$PWD/qeglfskmsgbmdevice.h \
           $$PWD/qeglfskmsgbmscreen.h \
           $$PWD/qeglfskmsgbmcursor.h \
           $$PWD/qeglfskmsgbmwindow.h

OTHER_FILES += $$PWD/eglfs_kms.json
