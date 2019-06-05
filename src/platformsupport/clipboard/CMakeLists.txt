# Generated from clipboard.pro.

#####################################################################
## ClipboardSupport Module:
#####################################################################

add_qt_module(ClipboardSupport
    STATIC
    INTERNAL_MODULE
    SOURCES
        qmacmime.mm qmacmime_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
    PUBLIC_LIBRARIES
        ${FWImageIO}
        Qt::Core
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:clipboard.pro:<TRUE>:
# CONFIG = "static" "internal_module"
# MODULE = "clipboard_support"
# _LOADED = "qt_module"

## Scopes:
#####################################################################

extend_target(ClipboardSupport CONDITION APPLE_OSX
    LIBRARIES
        ${FWAppKit}
)
