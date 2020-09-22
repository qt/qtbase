# Generated from qpinger.pro.

#####################################################################
## qpinger Binary:
#####################################################################

qt_internal_add_executable(qpinger
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    NO_INSTALL # special case
    SOURCES
        ../interface.cpp ../interface.h
        qpinger.cpp
    PUBLIC_LIBRARIES
        Qt::DBus
)
