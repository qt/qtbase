# Generated from server.pro.

#####################################################################
## server Binary:
#####################################################################

qt_add_benchmark(server
    GUI
    OUTPUT_DIRECTORY "."
    SOURCES
        ../serverobject.h
        server.cpp
    PUBLIC_LIBRARIES
        Qt::DBus
)
