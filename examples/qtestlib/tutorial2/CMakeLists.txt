# Generated from tutorial2.pro.

cmake_minimum_required(VERSION 3.14)
project(tutorial2 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)
find_package(Qt5 COMPONENTS Test)

add_executable(tutorial2 WIN32 MACOSX_BUNDLE
    testqstring.cpp
)
target_link_libraries(tutorial2 PUBLIC
    Qt::Test
    Qt::Widgets
)
install(TARGETS tutorial2
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
