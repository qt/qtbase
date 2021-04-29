# Generated from kmsconvenience.pro.

qt_find_package(Libdrm) # special case

#####################################################################
## KmsSupportPrivate Module:
#####################################################################

qt_internal_add_module(KmsSupportPrivate
    CONFIG_MODULE_NAME kms_support
    STATIC
    INTERNAL_MODULE
    SOURCES
        qkmsdevice.cpp qkmsdevice_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES
        Libdrm::Libdrm
        Qt::CorePrivate
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:.:kmsconvenience.pro:<TRUE>:
# MODULE = "kms_support"
