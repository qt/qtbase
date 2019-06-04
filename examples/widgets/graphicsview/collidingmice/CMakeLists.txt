# Generated from collidingmice.pro.

cmake_minimum_required(VERSION 3.14)
project(collidingmice LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(collidingmice WIN32 MACOSX_BUNDLE
    main.cpp
    mice.qrc
    mouse.cpp mouse.h
)
target_link_libraries(collidingmice PUBLIC
    Qt::Widgets
)

install(TARGETS collidingmice
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
