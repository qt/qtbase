# Generated from knobs.pro.

cmake_minimum_required(VERSION 3.14)
project(knobs LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(knobs WIN32 MACOSX_BUNDLE
    knob.cpp knob.h
    main.cpp
)
target_link_libraries(knobs PUBLIC
    Qt::Widgets
)

install(TARGETS knobs
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
