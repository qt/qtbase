# special case skip regeneration
cmake_minimum_required(VERSION 3.14)
project(dprgadget LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(dprgadget
    main.cpp
)

target_link_libraries(dprgadget PUBLIC
    Qt::Core
    Qt::Gui
    Qt::GuiPrivate
    Qt::Widgets
)

