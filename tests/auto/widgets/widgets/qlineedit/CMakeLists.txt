# Generated from qlineedit.pro.

#####################################################################
## tst_qlineedit Test:
#####################################################################

add_qt_test(tst_qlineedit
    SOURCES
        tst_qlineedit.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

## Scopes:
#####################################################################

extend_target(tst_qlineedit CONDITION APPLE_OSX
    PUBLIC_LIBRARIES
        ${FWAppKit}
)
