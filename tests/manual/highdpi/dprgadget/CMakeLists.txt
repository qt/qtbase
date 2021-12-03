# special case skip regeneration
cmake_minimum_required(VERSION 3.16)
project(dprgadget LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(dprgadget
    main.cpp
)

target_link_libraries(dprgadget PUBLIC
    Qt::Core
    Qt::Gui
    Qt::GuiPrivate
    Qt::Widgets
)

