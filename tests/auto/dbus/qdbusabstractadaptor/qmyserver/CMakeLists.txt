# Generated from qmyserver.pro.

#####################################################################
## qmyserver Binary:
#####################################################################

add_qt_executable(qmyserver
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    NO_INSTALL # special case
    SOURCES
        ../myobject.h
        qmyserver.cpp
    PUBLIC_LIBRARIES
        Qt::DBus
)
