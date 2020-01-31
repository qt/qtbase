# Generated from tooltips.pro.

cmake_minimum_required(VERSION 3.14)
project(tooltips LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(tooltips
    main.cpp
    shapeitem.cpp shapeitem.h
    sortingbox.cpp sortingbox.h
    tooltips.qrc
)
target_link_libraries(tooltips PUBLIC
    Qt::Widgets
)

install(TARGETS tooltips
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
