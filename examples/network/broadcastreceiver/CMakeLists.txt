# Generated from broadcastreceiver.pro.

#####################################################################
## broadcastreceiver Binary:
#####################################################################

add_qt_executable(broadcastreceiver
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/broadcastreceiver"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/broadcastreceiver"
    SOURCES
        main.cpp
        receiver.cpp receiver.h
    LIBRARIES
        Qt::Network
        Qt::Widgets
)
