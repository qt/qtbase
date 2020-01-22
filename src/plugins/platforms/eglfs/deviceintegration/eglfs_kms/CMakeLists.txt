# Generated from eglfs_kms.pro.

#####################################################################
## QEglFSKmsGbmIntegrationPlugin Plugin:
#####################################################################

qt_add_plugin(QEglFSKmsGbmIntegrationPlugin
    OUTPUT_NAME qeglfs-kms-integration
    TYPE egldeviceintegrations
    SOURCES
        qeglfskmsgbmcursor.cpp qeglfskmsgbmcursor.h
        qeglfskmsgbmdevice.cpp qeglfskmsgbmdevice.h
        qeglfskmsgbmintegration.cpp qeglfskmsgbmintegration.h
        qeglfskmsgbmmain.cpp
        qeglfskmsgbmscreen.cpp qeglfskmsgbmscreen.h
        qeglfskmsgbmwindow.cpp qeglfskmsgbmwindow.h
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
        gbm::gbm
)

#### Keys ignored in scope 1:.:.:eglfs_kms.pro:<TRUE>:
# OTHER_FILES = "$$PWD/eglfs_kms.json"
