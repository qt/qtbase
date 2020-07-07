# Generated from keypadnavigation.pro.

#####################################################################
## keypadnavigation Binary:
#####################################################################

qt_add_manual_test(keypadnavigation
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
