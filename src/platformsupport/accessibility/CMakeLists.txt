# Generated from accessibility.pro.

#####################################################################
## QtAccessibilitySupport Module:
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
)
