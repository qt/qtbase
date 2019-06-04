# Generated from qmyserver.pro.

#####################################################################
## qmyserver Binary:
#####################################################################

# Can't be called qmyserver since the qdbusabstractadaptor test
# already has a executable target named like that, so use
# a different target name and then rename the binary
add_qt_executable(qmyserver_qdbusinterface
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        ../myobject.h
        qmyserver.cpp
    LIBRARIES
        Qt::Core
        Qt::DBus
)
