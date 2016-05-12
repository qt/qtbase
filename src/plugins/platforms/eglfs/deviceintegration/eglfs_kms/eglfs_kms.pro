TARGET = qeglfs-kms-integration

PLUGIN_TYPE = egldeviceintegrations
PLUGIN_CLASS_NAME = QEglFSKmsGbmIntegrationPlugin
load(qt_plugin)

QT += core-private gui-private platformsupport-private eglfs_device_lib-private eglfs_kms_support-private

INCLUDEPATH += $$PWD/../.. $$PWD/../eglfs_kms_support

# Avoid X11 header collision
DEFINES += MESA_EGL_NO_X11_HEADERS

CONFIG += link_pkgconfig
!contains(QT_CONFIG, no-pkg-config) {
    PKGCONFIG += libdrm gbm
} else {
    LIBS += -ldrm -lgbm
}

CONFIG += egl
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

SOURCES += $$PWD/qeglfskmsgbmmain.cpp \
           $$PWD/qeglfskmsgbmintegration.cpp \
           $$PWD/qeglfskmsgbmdevice.cpp \
           $$PWD/qeglfskmsgbmscreen.cpp \
           $$PWD/qeglfskmsgbmcursor.cpp

HEADERS += $$PWD/qeglfskmsgbmintegration.h \
           $$PWD/qeglfskmsgbmdevice.h \
           $$PWD/qeglfskmsgbmscreen.h \
           $$PWD/qeglfskmsgbmcursor.h

OTHER_FILES += $$PWD/eglfs_kms.json
