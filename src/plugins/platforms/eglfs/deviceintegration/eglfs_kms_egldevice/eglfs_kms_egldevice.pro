TARGET = qeglfs-kms-egldevice-integration

QT += core-private gui-private platformsupport-private eglfs_device_lib-private eglfs_kms_support-private

INCLUDEPATH += $$PWD/../.. $$PWD/../eglfs_kms_support

DEFINES += MESA_EGL_NO_X11_HEADERS

CONFIG += link_pkgconfig
!contains(QT_CONFIG, no-pkg-config) {
    PKGCONFIG += libdrm
} else {
    LIBS += -ldrm
}

CONFIG += egl
QMAKE_LFLAGS += $$QMAKE_LFLAGS_NOUNDEF

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
