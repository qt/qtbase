# Generated from localfortuneserver.pro.

#####################################################################
## localfortuneserver Binary:
#####################################################################

add_qt_executable(localfortuneserver
    GUI
    OUTPUT_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/ipc/localfortuneserver"
    INSTALL_DIRECTORY "${INSTALL_EXAMPLESDIR}/corelib/ipc/localfortuneserver"
    SOURCES
        main.cpp
        server.cpp server.h
    LIBRARIES
        Qt::Network
        Qt::Widgets
)
