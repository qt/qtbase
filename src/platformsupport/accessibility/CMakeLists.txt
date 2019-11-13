# Generated from accessibility.pro.

#####################################################################
## AccessibilitySupport Module:
#####################################################################

qt_add_module(AccessibilitySupport
    STATIC
    INTERNAL_MODULE
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
# MODULE = "accessibility_support"
