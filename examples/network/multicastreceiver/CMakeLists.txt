# Generated from multicastreceiver.pro.

#####################################################################
## multicastreceiver Binary:
#####################################################################

add_qt_executable(multicastreceiver
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/multicastreceiver"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/multicastreceiver"
    SOURCES
        main.cpp
        receiver.cpp receiver.h
    LIBRARIES
        Qt::Network
        Qt::Widgets
)
