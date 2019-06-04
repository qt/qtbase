# Generated from flattreeview.pro.

cmake_minimum_required(VERSION 3.14)
project(flattreeview LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(flattreeview WIN32 MACOSX_BUNDLE
    main.cpp
)
target_link_libraries(flattreeview PUBLIC
    Qt::Widgets
)

install(TARGETS flattreeview
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
