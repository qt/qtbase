#####################################################################
## testForwarding Binary:
#####################################################################

qt_internal_add_executable(testForwarding
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        main.cpp
)

add_dependencies(testForwarding testForwardingHelper)
