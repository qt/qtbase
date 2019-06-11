# Generated from customtype.pro.

cmake_minimum_required(VERSION 3.14)
project(customtype LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(customtype WIN32 MACOSX_BUNDLE
    main.cpp
    message.cpp message.h
)
target_link_libraries(customtype PUBLIC
    Qt::Widgets
)
install(TARGETS customtype
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
