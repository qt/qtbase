# Generated from 4_headers.pro.

cmake_minimum_required(VERSION 3.14)
project(mv_headers LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(mv_headers
    main.cpp
    mymodel.cpp mymodel.h
)
target_link_libraries(mv_headers PUBLIC
    Qt::Widgets
)

install(TARGETS mv_headers
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
