# Generated from deform.pro.

cmake_minimum_required(VERSION 3.14)
project(deform LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(deform
    # special case: remove files from ../shared
    deform.qrc
    main.cpp
    pathdeform.cpp pathdeform.h
)
target_include_directories(deform PUBLIC
    ../shared
)
target_link_libraries(deform PUBLIC
    Qt::Widgets
    painting_shared # special case
)

install(TARGETS deform
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
