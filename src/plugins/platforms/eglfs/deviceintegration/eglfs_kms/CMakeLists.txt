# Generated from eglfs_kms.pro.

#####################################################################
## EglFsKmsGbmSupport Module:
#####################################################################

qt_internal_add_module(EglFsKmsGbmSupport
    INTERNAL_MODULE
    SOURCES
        qeglfskmsgbmcursor.cpp qeglfskmsgbmcursor_p.h
        qeglfskmsgbmdevice.cpp qeglfskmsgbmdevice_p.h
        qeglfskmsgbmintegration.cpp qeglfskmsgbmintegration_p.h
        qeglfskmsgbmscreen.cpp qeglfskmsgbmscreen_p.h
        qeglfskmsgbmwindow.cpp qeglfskmsgbmwindow_p.h
    DEFINES
        QT_EGL_NO_X11
    INCLUDE_DIRECTORIES
        ../../api
        ../eglfs_kms_support
    PUBLIC_LIBRARIES
        Libdrm::Libdrm
        Qt::Core
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::EglFsKmsSupportPrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::KmsSupportPrivate
        gbm::gbm
)

#####################################################################
## QEglFSKmsGbmIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEglFSKmsGbmIntegrationPlugin
    OUTPUT_NAME qeglfs-kms-integration
    TYPE egldeviceintegrations
    SOURCES
        qeglfskmsgbmmain.cpp
    DEFINES
        QT_EGL_NO_X11
    PUBLIC_LIBRARIES
        Libdrm::Libdrm
        Qt::Core
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::EglFsKmsGbmSupportPrivate
        Qt::EglFsKmsSupportPrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::KmsSupportPrivate
        gbm::gbm
)

#### Keys ignored in scope 1:.:.:eglfs_kms-plugin.pro:<TRUE>:
# OTHER_FILES = "$$PWD/eglfs_kms.json"
