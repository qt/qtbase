# Generated from udpServer.pro.

#####################################################################
## udpServer Binary:
#####################################################################

add_qt_executable(udpServer
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/./" # special case
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Network
)
