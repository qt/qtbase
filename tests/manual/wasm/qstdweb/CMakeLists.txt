qt_internal_add_manual_test(promise_auto
    SOURCES
        promise_main.cpp
        ../qtwasmtestlib/qtwasmtestlib.cpp
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::CorePrivate
)

include_directories(../qtwasmtestlib/)

add_custom_command(
    TARGET promise_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/promise_auto.html
            ${CMAKE_CURRENT_BINARY_DIR}/promise_auto.html)

add_custom_command(
    TARGET promise_auto POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../qtwasmtestlib/qtwasmtestlib.js
            ${CMAKE_CURRENT_BINARY_DIR}/qtwasmtestlib.js)
