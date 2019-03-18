# Generated from tutorial1.pro.

#####################################################################
## tutorial1 Binary:
#####################################################################

add_qt_executable(tutorial1
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtestlib/tutorial1"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtestlib/tutorial1"
    SOURCES
        testqstring.cpp
    LIBRARIES
        Qt::Widgets
        Qt::Test
)
