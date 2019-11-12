# Generated from lackey.pro.

#####################################################################
## lackey Binary:
#####################################################################

add_qt_executable(lackey
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Network
)
