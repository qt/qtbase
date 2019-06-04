# Generated from qpinger.pro.

#####################################################################
## qpinger Binary:
#####################################################################

add_qt_executable(qpinger
    NO_INSTALL # special case
    OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR} # special case
    SOURCES
        ../interface.cpp ../interface.h
        qpinger.cpp
    LIBRARIES
        Qt::Core
        Qt::DBus
)
