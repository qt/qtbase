# Generated from blockingfortuneclient.pro.

#####################################################################
## blockingfortuneclient Binary:
#####################################################################

add_qt_executable(blockingfortuneclient
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/blockingfortuneclient"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/network/blockingfortuneclient"
    SOURCES
        blockingclient.cpp blockingclient.h
        fortunethread.cpp fortunethread.h
        main.cpp
    LIBRARIES
        Qt::Network
        Qt::Widgets
)
