# Generated from qlineedit.pro.

#####################################################################
## tst_qlineedit Test:
#####################################################################

add_qt_test(tst_qlineedit
    SOURCES
        tst_qlineedit.cpp
    PUBLIC_LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::TestPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

extend_target(tst_qlineedit CONDITION APPLE_OSX
    PUBLIC_LIBRARIES
        ${FWAppKit}
)
