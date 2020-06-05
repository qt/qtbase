# Generated from testProcessLoopback.pro.

#####################################################################
## testProcessLoopback Binary:
#####################################################################

# add_qt_executable(testProcessLoopback # special case
add_qt_benchmark(testProcessLoopback # special case
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
    PUBLIC_LIBRARIES
        Qt::Gui
)
