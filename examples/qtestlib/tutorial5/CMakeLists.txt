# Generated from tutorial5.pro.

#####################################################################
## tutorial5 Binary:
#####################################################################

add_qt_executable(tutorial5
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtestlib/tutorial5"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/qtestlib/tutorial5"
    SOURCES
        benchmarking.cpp
    LIBRARIES
        Qt::Widgets
        Qt::Test
)
