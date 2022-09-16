include_directories(../../qtwasmtestlib/)

# default buid
qt_internal_add_manual_test(eventloop_auto
    SOURCES
        main.cpp
        ../../qtwasmtestlib/qtwasmtestlib.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
)

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

# asyncify enabled build
qt_internal_add_manual_test(eventloop_auto_asyncify
    SOURCES
        main.cpp
        ../../qtwasmtestlib/qtwasmtestlib.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
)

target_link_options(eventloop_auto_asyncify PRIVATE -sASYNCIFY -Os)

add_custom_command(
    TARGET eventloop_auto_asyncify POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/eventloop_auto_asyncify.html
            ${CMAKE_CURRENT_BINARY_DIR}/eventloop_auto_asyncify.html)


