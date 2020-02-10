# Generated from pingpong.pro.

cmake_minimum_required(VERSION 3.14)
project(pingpong LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/widgets/statemachine/pingpong")

find_package(Qt6 COMPONENTS Core)

add_executable(pingpong
    main.cpp
)
target_link_libraries(pingpong PUBLIC
    Qt::Core
)

install(TARGETS pingpong
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
