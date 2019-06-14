# Generated from flowlayout.pro.

cmake_minimum_required(VERSION 3.14)
project(flowlayout LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(graphicsview_flowlayout # special case: renamed flowlayout
    flowlayout.cpp flowlayout.h
    main.cpp
    window.cpp window.h
)
target_link_libraries(graphicsview_flowlayout PUBLIC # special case: renamed flowlayout
    Qt::Widgets
)

install(TARGETS graphicsview_flowlayout # special case: renamed flowlayout
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
