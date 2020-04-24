# Generated from eglfs_kms.pro.

#####################################################################
## QEglFSKmsGbmIntegrationPlugin Plugin:
#####################################################################

qt_internal_add_plugin(QEglFSKmsGbmIntegrationPlugin
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
    PUBLIC_LIBRARIES
        Libdrm::Libdrm
        Qt::Core
        Qt::CorePrivate
        Qt::EdidSupportPrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::EglFsKmsSupportPrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::KmsSupportPrivate
        gbm::gbm
)

#### Keys ignored in scope 1:.:.:eglfs_kms.pro:<TRUE>:
# OTHER_FILES = "$$PWD/eglfs_kms.json"
