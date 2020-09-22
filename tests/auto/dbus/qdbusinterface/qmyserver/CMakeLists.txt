# Generated from qmyserver.pro.

#####################################################################
## qmyserver_qdbusinterface Binary:
#####################################################################

qt_internal_add_executable(qmyserver_qdbusinterface
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    NO_INSTALL # special case
    SOURCES
        ../myobject.h
        qmyserver.cpp
    PUBLIC_LIBRARIES
        Qt::DBus
)
