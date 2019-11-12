# Generated from qpong.pro.

#####################################################################
## qpong Binary:
#####################################################################

add_qt_executable(qpong
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        qpong.cpp
    PUBLIC_LIBRARIES
        Qt::DBus
)
