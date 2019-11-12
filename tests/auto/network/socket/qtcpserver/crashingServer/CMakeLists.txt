# Generated from crashingServer.pro.

#####################################################################
## crashingServer Binary:
#####################################################################

add_qt_executable(crashingServer
    GUI
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Network
)

## Scopes:
#####################################################################
