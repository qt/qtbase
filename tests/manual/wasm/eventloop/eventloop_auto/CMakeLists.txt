qt_internal_add_manual_test(eventloop_auto
    SOURCES
        main.cpp
        ../../qtwasmtestlib/qtwasmtestlib.cpp
    PUBLIC_LIBRARIES
        Qt::Core
)

include_directories(../../qtwasmtestlib/)

add_custom_command(
    TARGET eventloop_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/eventloop_auto.html
            ${CMAKE_CURRENT_BINARY_DIR}/eventloop_auto.html)

add_custom_command(
    TARGET eventloop_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../qtwasmtestlib/qtwasmtestlib.js
            ${CMAKE_CURRENT_BINARY_DIR}/qtwasmtestlib.js)
