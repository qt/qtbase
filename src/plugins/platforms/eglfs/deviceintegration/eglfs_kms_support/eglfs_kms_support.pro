TARGET = QtEglFsKmsSupport
CONFIG += no_module_headers internal_module
load(qt_module)

QT += core-private gui-private platformsupport-private eglfs_device_lib-private

INCLUDEPATH += $$PWD/../..

# Avoid X11 header collision, use generic EGL native types
DEFINES += QT_EGL_NO_X11

CONFIG += link_pkgconfig
!contains(QT_CONFIG, no-pkg-config) {
    PKGCONFIG += libdrm
} else {
    LIBS += -ldrm
}

CONFIG += egl
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

SOURCES += $$PWD/qeglfskmsintegration.cpp \
           $$PWD/qeglfskmsdevice.cpp \
           $$PWD/qeglfskmsscreen.cpp \

HEADERS += $$PWD/qeglfskmsintegration.h \
           $$PWD/qeglfskmsdevice.h \
           $$PWD/qeglfskmsscreen.h \
