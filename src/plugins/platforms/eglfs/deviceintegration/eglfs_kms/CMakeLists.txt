# Generated from eglfs_kms.pro.

#####################################################################
## EglFsKmsGbmSupportPrivate Module:
#####################################################################

qt_internal_add_module(EglFsKmsGbmSupportPrivate
    CONFIG_MODULE_NAME eglfs_kms_gbm_support
    INTERNAL_MODULE
    SOURCES
        qeglfskmsgbmcursor.cpp qeglfskmsgbmcursor_p.h
        qeglfskmsgbmdevice.cpp qeglfskmsgbmdevice_p.h
        qeglfskmsgbmintegration.cpp qeglfskmsgbmintegration_p.h
        qeglfskmsgbmscreen.cpp qeglfskmsgbmscreen_p.h
        qeglfskmsgbmwindow.cpp qeglfskmsgbmwindow_p.h
    DEFINES
        QT_EGL_NO_X11
    PUBLIC_LIBRARIES
        Libdrm::Libdrm
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::EglFsKmsSupportPrivate
        Qt::GuiPrivate
        Qt::KmsSupportPrivate
        gbm::gbm
)
#####################################################################
## QEglFSKmsGbmIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEglFSKmsGbmIntegrationPlugin
    OUTPUT_NAME qeglfs-kms-integration
    PLUGIN_TYPE egldeviceintegrations
    SOURCES
        qeglfskmsgbmmain.cpp
    DEFINES
        QT_EGL_NO_X11
    LIBRARIES
        Libdrm::Libdrm
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::EglFsKmsGbmSupportPrivate
        Qt::EglFsKmsSupportPrivate
        Qt::GuiPrivate
        Qt::KmsSupportPrivate
        gbm::gbm
)

#### Keys ignored in scope 3:.:.:eglfs_kms-plugin.pro:<TRUE>:
# OTHER_FILES = "$$PWD/eglfs_kms.json"
