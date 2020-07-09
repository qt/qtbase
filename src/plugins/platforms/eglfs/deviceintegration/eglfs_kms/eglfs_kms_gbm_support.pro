TARGET = QtEglFsKmsGbmSupport
CONFIG += internal_module
load(qt_module)

QT += core-private gui-private eglfsdeviceintegration-private eglfs_kms_support-private kms_support-private

INCLUDEPATH += $$PWD/../../api $$PWD/../eglfs_kms_support

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

QMAKE_USE += gbm drm
CONFIG += egl

SOURCES += $$PWD/qeglfskmsgbmintegration.cpp \
           $$PWD/qeglfskmsgbmdevice.cpp \
           $$PWD/qeglfskmsgbmscreen.cpp \
           $$PWD/qeglfskmsgbmcursor.cpp \
           $$PWD/qeglfskmsgbmwindow.cpp

HEADERS += $$PWD/qeglfskmsgbmintegration_p.h \
           $$PWD/qeglfskmsgbmdevice_p.h \
           $$PWD/qeglfskmsgbmscreen_p.h \
           $$PWD/qeglfskmsgbmcursor_p.h \
           $$PWD/qeglfskmsgbmwindow_p.h
