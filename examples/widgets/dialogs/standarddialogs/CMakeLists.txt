# Generated from standarddialogs.pro.

cmake_minimum_required(VERSION 3.14)
project(standarddialogs LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(standarddialogs
    dialog.cpp dialog.h
    main.cpp
)
target_link_libraries(standarddialogs PUBLIC
    Qt::Widgets
)

install(TARGETS standarddialogs
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
