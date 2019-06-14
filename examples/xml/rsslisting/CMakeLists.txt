# Generated from rsslisting.pro.

cmake_minimum_required(VERSION 3.14)
project(rsslisting LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Network)
find_package(Qt6 COMPONENTS Xml)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(rsslisting
    main.cpp
    rsslisting.cpp rsslisting.h
)
target_link_libraries(rsslisting PUBLIC
    Qt::Network
    Qt::Widgets
    Qt::Xml
)
install(TARGETS rsslisting
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
