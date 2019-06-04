# Generated from 5_edit.pro.

cmake_minimum_required(VERSION 3.14)
project(mv_edit LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(mv_edit WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp mainwindow.h
    mymodel.cpp mymodel.h
)
target_link_libraries(mv_edit PUBLIC
    Qt::Widgets
)

install(TARGETS mv_edit
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
