# Generated from windowmodality.pro.

#####################################################################
## windowmodality Binary:
#####################################################################

qt_internal_add_manual_test(windowmodality
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
