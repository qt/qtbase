# Generated from trivialwizard.pro.

#####################################################################
## trivialwizard Binary:
#####################################################################

add_qt_executable(trivialwizard
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/dialogs/trivialwizard"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/widgets/dialogs/trivialwizard"
    SOURCES
        trivialwizard.cpp
    LIBRARIES
        Qt::Widgets
)
