# Generated from qplaintextedit.pro.

#####################################################################
## tst_qplaintextedit Test:
#####################################################################

add_qt_test(tst_qplaintextedit
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

extend_target(tst_qplaintextedit CONDITION APPLE_OSX
    PUBLIC_LIBRARIES
        ${FWAppKit}
)
