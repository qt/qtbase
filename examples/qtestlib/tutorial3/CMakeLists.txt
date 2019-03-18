# Generated from tutorial3.pro.

#####################################################################
## tutorial3 Binary:
#####################################################################

add_qt_executable(tutorial3
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtestlib/tutorial3"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtestlib/tutorial3"
    SOURCES
        testgui.cpp
    LIBRARIES
        Qt::Widgets
        Qt::Test
)
