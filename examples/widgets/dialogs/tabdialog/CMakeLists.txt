# Generated from tabdialog.pro.

#####################################################################
## tabdialog Binary:
#####################################################################

add_qt_executable(tabdialog
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/dialogs/tabdialog"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/dialogs/tabdialog"
    SOURCES
        main.cpp
        tabdialog.cpp tabdialog.h
    LIBRARIES
        Qt::Widgets
)
