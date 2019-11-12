# Generated from windowmodality.pro.

#####################################################################
## windowmodality Binary:
#####################################################################

add_qt_manual_test(windowmodality
    GUI
    SOURCES
        dialog.ui
        main.cpp
        widget.ui
    PUBLIC_LIBRARIES
        Qt::Gui
        Qt::PrintSupport
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)
