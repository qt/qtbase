# Generated from googlesuggest.pro.

cmake_minimum_required(VERSION 3.14)
project(googlesuggest LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Network)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(googlesuggest
    googlesuggest.cpp googlesuggest.h
    main.cpp
    searchbox.cpp searchbox.h
)
target_link_libraries(googlesuggest PUBLIC
    Qt::Network
    Qt::Widgets
)

install(TARGETS googlesuggest
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
