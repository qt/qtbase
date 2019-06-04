# Generated from 2_formatting.pro.

cmake_minimum_required(VERSION 3.14)
project(mv_formatting LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(mv_formatting WIN32 MACOSX_BUNDLE
    main.cpp
    mymodel.cpp mymodel.h
)
target_link_libraries(mv_formatting PUBLIC
    Qt::Widgets
)

install(TARGETS mv_formatting
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
