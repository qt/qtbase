# Generated from tutorial2.pro.

#####################################################################
## tutorial2 Binary:
#####################################################################

add_qt_executable(tutorial2
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtestlib/tutorial2"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtestlib/tutorial2"
    SOURCES
        testqstring.cpp
    LIBRARIES
        Qt::Widgets
        Qt::Test
)
