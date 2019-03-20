# Generated from extension.pro.

#####################################################################
## extension Binary:
#####################################################################

add_qt_executable(extension
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/dialogs/extension"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/dialogs/extension"
    SOURCES
        finddialog.cpp finddialog.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)
