# Generated from network-chat.pro.

#####################################################################
## network-chat Binary:
#####################################################################

add_qt_executable(network-chat
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/network-chat"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/network-chat"
    SOURCES
        chatdialog.cpp chatdialog.h chatdialog.ui
        client.cpp client.h
        connection.cpp connection.h
        main.cpp
        peermanager.cpp peermanager.h
        server.cpp server.h
    LIBRARIES
        Qt::Network
        Qt::Widgets
)
