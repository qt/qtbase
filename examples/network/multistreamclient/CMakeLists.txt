# Generated from multistreamclient.pro.

#####################################################################
## multistreamclient Binary:
#####################################################################

add_qt_executable(multistreamclient
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/multistreamclient"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/multistreamclient"
    SOURCES
        chatconsumer.cpp chatconsumer.h
        client.cpp client.h
        consumer.h
        main.cpp
        movieconsumer.cpp movieconsumer.h
        timeconsumer.cpp timeconsumer.h
    LIBRARIES
        Qt::Network
        Qt::Widgets
)
