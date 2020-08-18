# Generated from eglfs_kms_support.pro.

#####################################################################
## EglFsKmsSupport Module:
#####################################################################

qt_add_module(EglFsKmsSupport
    INTERNAL_MODULE
    NO_MODULE_HEADERS
    SOURCES
        qeglfskmsdevice.cpp qeglfskmsdevice.h
        qeglfskmseventreader.cpp qeglfskmseventreader.h
        qeglfskmshelpers.h
        qeglfskmsintegration.cpp qeglfskmsintegration.h
        qeglfskmsscreen.cpp qeglfskmsscreen.h
    DEFINES
        QT_EGL_NO_X11
    INCLUDE_DIRECTORIES
        ../../api
    PUBLIC_LIBRARIES
        Libdrm::Libdrm
        Qt::Core
        Qt::CorePrivate
        Qt::EglFSDeviceIntegrationPrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::KmsSupportPrivate
)

# begin special case
qt_extend_target(EglFsKmsSupport
      CONDITION QNX
      COMPILE_OPTIONS -fpermissive
)
# end special case
