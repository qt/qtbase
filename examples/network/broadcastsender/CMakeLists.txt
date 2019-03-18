# Generated from broadcastsender.pro.

#####################################################################
## broadcastsender Binary:
#####################################################################

add_qt_executable(broadcastsender
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/broadcastsender"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/broadcastsender"
    SOURCES
        main.cpp
        sender.cpp sender.h
    LIBRARIES
        Qt::Network
        Qt::Widgets
)
