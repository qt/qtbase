# special case skip regeneration

cmake_minimum_required(VERSION 3.14.0)

project(Parser CXX)
find_package(Qt6 REQUIRED COMPONENTS Core Gui Xml)

add_executable(parser)

target_sources(parser PRIVATE
    main.cpp
    parser.cpp
    parser.h
)

target_link_libraries(parser PRIVATE
    Qt::Gui
    Qt::Xml
)
