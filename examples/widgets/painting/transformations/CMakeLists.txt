# Generated from transformations.pro.

cmake_minimum_required(VERSION 3.14)
project(transformations LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(transformations WIN32 MACOSX_BUNDLE
    main.cpp
    renderarea.cpp renderarea.h
    window.cpp window.h
)
target_link_libraries(transformations PUBLIC
    Qt::Widgets
)

install(TARGETS transformations
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
