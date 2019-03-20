# Generated from standarddialogs.pro.

#####################################################################
## standarddialogs Binary:
#####################################################################

add_qt_executable(standarddialogs
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/dialogs/standarddialogs"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/dialogs/standarddialogs"
    SOURCES
        dialog.cpp dialog.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)
