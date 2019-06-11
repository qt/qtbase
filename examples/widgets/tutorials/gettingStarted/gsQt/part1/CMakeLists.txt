# Generated from part1.pro.

cmake_minimum_required(VERSION 3.14)
project(part1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(part1 WIN32 MACOSX_BUNDLE
    main.cpp
)
target_link_libraries(part1 PUBLIC
    Qt::Widgets
)

install(TARGETS part1
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
