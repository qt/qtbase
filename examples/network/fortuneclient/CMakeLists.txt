# Generated from fortuneclient.pro.

#####################################################################
## fortuneclient Binary:
#####################################################################

add_qt_executable(fortuneclient
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/fortuneclient"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/fortuneclient"
    SOURCES
        client.cpp client.h
        main.cpp
    LIBRARIES
        Qt::Network
        Qt::Widgets
)
