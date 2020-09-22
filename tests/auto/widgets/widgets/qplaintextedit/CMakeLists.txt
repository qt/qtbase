# Generated from qplaintextedit.pro.

#####################################################################
## tst_qplaintextedit Test:
#####################################################################

qt_internal_add_test(tst_qplaintextedit
    SOURCES
        tst_qplaintextedit.cpp
    INCLUDE_DIRECTORIES
        ..
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::GuiPrivate
        Qt::Widgets
        Qt::WidgetsPrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qplaintextedit CONDITION MACOS
    PUBLIC_LIBRARIES
        ${FWAppKit}
)
