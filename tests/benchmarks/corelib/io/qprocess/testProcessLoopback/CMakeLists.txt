# Generated from testProcessLoopback.pro.

#####################################################################
## testProcessLoopback Binary:
#####################################################################

add_qt_benchmark(testProcessLoopback
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)

## Scopes:
#####################################################################

extend_target(testProcessLoopback CONDITION WINRT
    LINK_OPTIONS
        "/ENTRY:mainCRTStartup"
)
