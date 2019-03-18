# Generated from tutorial4.pro.

#####################################################################
## tutorial4 Binary:
#####################################################################

add_qt_executable(tutorial4
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtestlib/tutorial4"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtestlib/tutorial4"
    SOURCES
        testgui.cpp
    LIBRARIES
        Qt::Widgets
        Qt::Test
)
