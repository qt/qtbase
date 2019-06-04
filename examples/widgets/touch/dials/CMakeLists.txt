# Generated from dials.pro.

cmake_minimum_required(VERSION 3.14)
project(dials LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(dials WIN32 MACOSX_BUNDLE
    dials.ui
    main.cpp
)
target_link_libraries(dials PUBLIC
    Qt::Widgets
)

install(TARGETS dials
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
