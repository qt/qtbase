# Generated from keypadnavigation.pro.

#####################################################################
## keypadnavigation Binary:
#####################################################################

add_qt_manual_test(keypadnavigation
    GUI
    SOURCES
        keypadnavigation.ui
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
