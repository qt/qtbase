# Generated from flowlayout.pro.

cmake_minimum_required(VERSION 3.14)
project(flowlayout LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(flowlayout WIN32 MACOSX_BUNDLE
    flowlayout.cpp flowlayout.h
    main.cpp
    window.cpp window.h
)
target_link_libraries(flowlayout PUBLIC
    Qt::Widgets
)

install(TARGETS flowlayout
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
