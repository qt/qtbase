# Generated from xmlstreamlint.pro.

cmake_minimum_required(VERSION 3.14)
project(xmlstreamlint LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/xml/xmlstreamlint")

find_package(Qt6 COMPONENTS Core Xml) # special case

add_executable(xmlstreamlint
    main.cpp
)
target_link_libraries(xmlstreamlint PUBLIC
    Qt::Core
    Qt::Xml # special case: add
)

install(TARGETS xmlstreamlint
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
