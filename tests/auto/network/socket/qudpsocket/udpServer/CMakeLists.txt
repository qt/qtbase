# Generated from udpServer.pro.

#####################################################################
## udpServer Binary:
#####################################################################

qt_internal_add_executable(udpServer
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/./" # special case
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Network
)
