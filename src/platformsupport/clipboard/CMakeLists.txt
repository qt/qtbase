# Generated from clipboard.pro.

#####################################################################
## QtClipboardSupport Module:
#####################################################################

find_library(FWAppKit AppKit)
find_library(FWImageIO ImageIO)
find_library(FWCoreFoundation CoreFoundation)

add_qt_module(ClipboardSupport
    STATIC
    SOURCES
        qmacmime.mm qmacmime_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
)

## Scopes:
#####################################################################

extend_target(ClipboardSupport CONDITION APPLE
    LIBRARIES ${FWAppKit} ${FWImageIO} ${FWCoreFoundation}
)
