# Generated from eglfs_kms_support.pro.

find_package(Libdrm)

#####################################################################
## EglFsKmsSupport Module:
#####################################################################

add_qt_module(EglFsKmsSupport
    NO_MODULE_HEADERS
    SOURCES
        qeglfskmsdevice.cpp qeglfskmsdevice.h
        qeglfskmshelpers.h
        qeglfskmsintegration.cpp qeglfskmsintegration.h
        qeglfskmsscreen.cpp qeglfskmsscreen.h
    DEFINES
        QT_EGL_NO_X11
    INCLUDE_DIRECTORIES
        ../../api
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::KmsSupportPrivate
        Qt::EdidSupportPrivate
        Libdrm::Libdrm
    # CONFIG = "no_module_headers" "internal_module" "egl"
    # _LOADED = "qt_module"
)
