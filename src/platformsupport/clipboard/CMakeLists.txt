# Generated from clipboard.pro.

#####################################################################
## ClipboardSupport Module:
#####################################################################

qt_add_module(ClipboardSupport
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
# MODULE = "clipboard_support"

## Scopes:
#####################################################################

qt_extend_target(ClipboardSupport CONDITION APPLE_OSX
    LIBRARIES
        ${FWAppKit}
)
