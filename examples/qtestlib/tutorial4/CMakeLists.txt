# Generated from tutorial4.pro.

cmake_minimum_required(VERSION 3.14)
project(tutorial4 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)
find_package(Qt5 COMPONENTS Test)

add_qt_gui_executable(tutorial4 WIN32 MACOSX_BUNDLE
    testgui.cpp
)
target_link_libraries(tutorial4 PUBLIC
    Qt::Test
    Qt::Widgets
)
install(TARGETS tutorial4
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
