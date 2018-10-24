# Generated from qpinger.pro.

#####################################################################
## qpinger Binary:
#####################################################################

add_qt_executable(qpinger
    SOURCES
        ../interface.cpp ../interface.h
        qpinger.cpp
    LIBRARIES
        Qt::DBus
)
