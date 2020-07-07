# Generated from qpong.pro.

#####################################################################
## qpong Binary:
#####################################################################

qt_add_executable(qpong
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        qpong.cpp
    PUBLIC_LIBRARIES
        Qt::DBus
)
