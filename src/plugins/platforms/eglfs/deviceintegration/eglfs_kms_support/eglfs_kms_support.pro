TARGET = QtEglFsKmsSupport
CONFIG += internal_module
load(qt_module)

QT += core-private gui-private eglfsdeviceintegration-private kms_support-private

INCLUDEPATH += $$PWD/../../api

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

QMAKE_USE += drm
CONFIG += egl

SOURCES += $$PWD/qeglfskmsintegration.cpp \
           $$PWD/qeglfskmsdevice.cpp \
           $$PWD/qeglfskmsscreen.cpp \
           $$PWD/qeglfskmseventreader.cpp

HEADERS += $$PWD/qeglfskmsintegration_p.h \
           $$PWD/qeglfskmsdevice_p.h \
           $$PWD/qeglfskmsscreen_p.h \
           $$PWD/qeglfskmshelpers_p.h \
           $$PWD/qeglfskmseventreader_p.h
