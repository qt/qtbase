find_package(Libdrm) # special case

#####################################################################
## KmsSupport Module:
#####################################################################

add_qt_module(KmsSupport
    STATIC
    SOURCES
        qkmsdevice.cpp qkmsdevice_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    PUBLIC_LIBRARIES # special case
        Libdrm::Libdrm
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
)

#### Keys ignored in scope 1:.:kmsconvenience.pro:<NONE>:
# CONFIG = "static" "internal_module"
# MODULE = "kms_support"
# _LOADED = "qt_module"
