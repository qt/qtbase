# Generated from eglfs_kms_support.pro.

#####################################################################
## EglFsKmsSupportPrivate Module:
#####################################################################

qt_internal_add_module(EglFsKmsSupportPrivate
    CONFIG_MODULE_NAME eglfs_kms_support
    INTERNAL_MODULE
    SOURCES
        qeglfskmsdevice.cpp qeglfskmsdevice_p.h
        qeglfskmseventreader.cpp qeglfskmseventreader_p.h
        qeglfskmshelpers_p.h
        qeglfskmsintegration.cpp qeglfskmsintegration_p.h
        qeglfskmsscreen.cpp qeglfskmsscreen_p.h
    DEFINES
        QT_EGL_NO_X11
    PUBLIC_LIBRARIES
        Libdrm::Libdrm
        Qt::Core
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::KmsSupportPrivate
)
