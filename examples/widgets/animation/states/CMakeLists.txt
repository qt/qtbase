# Generated from states.pro.

cmake_minimum_required(VERSION 3.14)
project(states LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(states WIN32 MACOSX_BUNDLE
    main.cpp
    states.qrc
)
target_link_libraries(states PUBLIC
    Qt::Widgets
)

install(TARGETS states
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
