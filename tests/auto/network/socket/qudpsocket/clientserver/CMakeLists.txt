# Generated from clientserver.pro.

#####################################################################
## clientserver Binary:
#####################################################################

qt_add_executable(clientserver
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Network
)
