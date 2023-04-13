qt_internal_add_manual_test(qwasmwindow_harness
    SOURCES
        qwasmwindow_harness.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::GuiPrivate
)

add_custom_command(
    TARGET qwasmwindow_harness POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/qwasmwindow_harness.html
            ${CMAKE_CURRENT_BINARY_DIR}/qwasmwindow_harness.html
)

add_custom_command(
    TARGET qwasmwindow_harness POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/qwasmwindow.py
            ${CMAKE_CURRENT_BINARY_DIR}/qwasmwindow.py
)

add_custom_command(
    TARGET qwasmwindow_harness POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/run.sh
            ${CMAKE_CURRENT_BINARY_DIR}/run.sh
)

add_custom_command(
    TARGET qwasmwindow_harness POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
            ${CMAKE_CURRENT_SOURCE_DIR}/../../../../util/wasm/qtwasmserver/qtwasmserver.py
            ${CMAKE_CURRENT_BINARY_DIR}/qtwasmserver.py
)
