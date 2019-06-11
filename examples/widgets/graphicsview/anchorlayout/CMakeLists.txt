# Generated from anchorlayout.pro.

cmake_minimum_required(VERSION 3.14)
project(anchorlayout LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(anchorlayout WIN32 MACOSX_BUNDLE
    main.cpp
)
target_link_libraries(anchorlayout PUBLIC
    Qt::Widgets
)

install(TARGETS anchorlayout
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
