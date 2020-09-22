# Generated from socketprocess.pro.

#####################################################################
## socketprocess Binary:
#####################################################################

qt_internal_add_executable(socketprocess
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Network
        Qt::Test
)
