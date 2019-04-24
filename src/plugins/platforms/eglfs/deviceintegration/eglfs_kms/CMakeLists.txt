# Generated from eglfs_kms.pro.

qt_find_package(gbm)
qt_find_package(Libdrm)

#####################################################################
## qeglfs-kms-integration Plugin:
#####################################################################

add_qt_plugin(qeglfs-kms-integration
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
        Qt::GuiPrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::EglFsKmsSupportPrivate
        Qt::KmsSupportPrivate
        Qt::EdidSupportPrivate
        Qt::PlatformCompositorSupportPrivate
        gbm::gbm
        Libdrm::Libdrm
    # CONFIG = "egl"
    # OTHER_FILES = "$$PWD/eglfs_kms.json"
    # PLUGIN_CLASS_NAME = "QEglFSKmsGbmIntegrationPlugin"
    # _LOADED = "qt_plugin"
)
