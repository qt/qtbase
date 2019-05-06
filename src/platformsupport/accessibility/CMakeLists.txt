# Generated from accessibility.pro.

#####################################################################
## AccessibilitySupport Module:
#####################################################################

add_qt_module(AccessibilitySupport
    STATIC
    SOURCES
        qaccessiblebridgeutils.cpp qaccessiblebridgeutils_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
)

#### Keys ignored in scope 1:.:.:accessibility.pro:<TRUE>:
# CONFIG = "static" "internal_module"
# MODULE = "accessibility_support"
# _LOADED = "qt_module"
