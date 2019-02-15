# Generated from eglfs_kms_egldevice.pro.

find_package(Libdrm)

#####################################################################
## qeglfs-kms-egldevice-integration Plugin:
#####################################################################

add_qt_plugin(qeglfs-kms-egldevice-integration
    TYPE egldeviceintegrations
    SOURCES
        qeglfskmsegldevice.cpp qeglfskmsegldevice.h
        qeglfskmsegldeviceintegration.cpp qeglfskmsegldeviceintegration.h
        qeglfskmsegldevicemain.cpp
        qeglfskmsegldevicescreen.cpp qeglfskmsegldevicescreen.h
    DEFINES
        QT_EGL_NO_X11
    INCLUDE_DIRECTORIES
        ../../api
        ../eglfs_kms_support
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::EglFsKmsSupportPrivate
        Qt::KmsSupportPrivate
        Qt::EdidSupportPrivate
        Qt::PlatformCompositorSupportPrivate
        Libdrm::Libdrm
    # CONFIG = "egl"
    # OTHER_FILES = "$$PWD/eglfs_kms_egldevice.json"
    # PLUGIN_CLASS_NAME = "QEglFSKmsEglDeviceIntegrationPlugin"
    # _LOADED = "qt_plugin"
)
