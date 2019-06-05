# Generated from eglfs_kms_egldevice.pro.

#####################################################################
## qeglfs-kms-egldevice-integration Plugin:
#####################################################################

add_qt_plugin(qeglfs-kms-egldevice-integration
    TYPE egldeviceintegrations
    CLASS_NAME QEglFSKmsEglDeviceIntegrationPlugin
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
        Qt::EdidSupportPrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::EglFsKmsSupportPrivate
        Qt::GuiPrivate
        Qt::KmsSupportPrivate
    PUBLIC_LIBRARIES
        Libdrm::Libdrm
        Qt::Core
        Qt::EdidSupport
        Qt::EglFSDeviceIntegration
        Qt::EglFsKmsSupport
        Qt::Gui
        Qt::KmsSupport
)

#### Keys ignored in scope 1:.:.:eglfs_kms_egldevice.pro:<TRUE>:
# CONFIG = "egl"
# OTHER_FILES = "$$PWD/eglfs_kms_egldevice.json"
# PLUGIN_CLASS_NAME = "QEglFSKmsEglDeviceIntegrationPlugin"
# PLUGIN_TYPE = "egldeviceintegrations"
# _LOADED = "qt_plugin"
