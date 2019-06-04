# Generated from flickable.pro.

cmake_minimum_required(VERSION 3.14)
project(flickable LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(flickable WIN32 MACOSX_BUNDLE
    flickable.cpp flickable.h
    main.cpp
)
target_link_libraries(flickable PUBLIC
    Qt::Widgets
)
install(TARGETS flickable
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
