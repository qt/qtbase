# Generated from qmyserver.pro.

#####################################################################
## qmyserver Binary:
#####################################################################

add_qt_executable(qmyserver
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        ../myobject.h
        qmyserver.cpp
    LIBRARIES
        Qt::Core
        Qt::DBus
)
