# Generated from codeeditor.pro.

cmake_minimum_required(VERSION 3.14)
project(codeeditor LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(codeeditor WIN32 MACOSX_BUNDLE
    codeeditor.cpp codeeditor.h
    main.cpp
)
target_link_libraries(codeeditor PUBLIC
    Qt::Widgets
)

install(TARGETS codeeditor
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
