# Generated from qlineedit.pro.

#####################################################################
## tst_qlineedit Test:
#####################################################################

qt_internal_add_test(tst_qlineedit
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

qt_internal_extend_target(tst_qlineedit CONDITION MACOS
    PUBLIC_LIBRARIES
        ${FWAppKit}
)
