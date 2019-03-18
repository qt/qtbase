# Generated from loopback.pro.

#####################################################################
## loopback Binary:
#####################################################################

add_qt_executable(loopback
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/loopback"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/loopback"
    SOURCES
        dialog.cpp dialog.h
        main.cpp
    LIBRARIES
        Qt::Network
        Qt::Widgets
)
