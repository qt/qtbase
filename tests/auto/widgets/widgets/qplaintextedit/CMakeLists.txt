# Generated from qplaintextedit.pro.

#####################################################################
## tst_qplaintextedit Test:
#####################################################################

add_qt_test(tst_qplaintextedit
    SOURCES
        tst_qplaintextedit.cpp
    INCLUDE_DIRECTORIES
        ..
    LIBRARIES
        Qt::GuiPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
)

## Scopes:
#####################################################################

extend_target(tst_qplaintextedit CONDITION APPLE_OSX
    PUBLIC_LIBRARIES
        ${FWAppKit}
)
