# Generated from server.pro.

#####################################################################
## server Binary:
#####################################################################

qt_internal_add_executable(server
    GUI
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        ../serverobject.h
        server.cpp
    PUBLIC_LIBRARIES
        Qt::DBus
)
