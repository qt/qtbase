# Generated from affine.pro.

cmake_minimum_required(VERSION 3.14)
project(affine LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(affine
    # special case: remove files from ../shared
    affine.qrc
    main.cpp
    xform.cpp xform.h
)
target_include_directories(affine PUBLIC
    ../shared
)
target_link_libraries(affine PUBLIC
    Qt::Widgets
    painting_shared # special case
)

install(TARGETS affine
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
