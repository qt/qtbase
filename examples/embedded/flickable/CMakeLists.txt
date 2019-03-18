# Generated from flickable.pro.

#####################################################################
## flickable Binary:
#####################################################################

add_qt_executable(flickable
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/embedded/flickable"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/embedded/flickable"
    SOURCES
        flickable.cpp flickable.h
        main.cpp
    LIBRARIES
        Qt::Widgets
)
