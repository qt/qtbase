# Generated from clientserver.pro.

#####################################################################
## clientserver Binary:
#####################################################################

add_qt_executable(clientserver
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Network
)
