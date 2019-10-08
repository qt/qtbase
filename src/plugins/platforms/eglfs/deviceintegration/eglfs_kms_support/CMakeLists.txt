# Generated from eglfs_kms_support.pro.

#####################################################################
## EglFsKmsSupport Module:
#####################################################################

add_qt_module(EglFsKmsSupport
    INTERNAL_MODULE
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
        Qt::EdidSupportPrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::GuiPrivate
        Qt::KmsSupportPrivate
    PUBLIC_LIBRARIES
        Libdrm::Libdrm
        Qt::Core
        Qt::EdidSupport
        Qt::EglFSDeviceIntegration
        Qt::Gui
        Qt::KmsSupport
)

#### Keys ignored in scope 1:.:.:eglfs_kms_support.pro:<TRUE>:
# _LOADED = "qt_module"
