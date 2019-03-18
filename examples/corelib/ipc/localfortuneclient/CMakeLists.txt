# Generated from localfortuneclient.pro.

#####################################################################
## localfortuneclient Binary:
#####################################################################

add_qt_executable(localfortuneclient
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/ipc/localfortuneclient"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/ipc/localfortuneclient"
    SOURCES
        client.cpp client.h
        main.cpp
    LIBRARIES
        Qt::Network
        Qt::Widgets
)
