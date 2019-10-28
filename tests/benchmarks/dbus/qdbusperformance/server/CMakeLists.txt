# Generated from server.pro.

#####################################################################
## server Binary:
#####################################################################

add_qt_benchmark(server
    GUI
    OUTPUT_DIRECTORY "."
    SOURCES
        ../serverobject.h
        server.cpp
    PUBLIC_LIBRARIES
        Qt::DBus
)
