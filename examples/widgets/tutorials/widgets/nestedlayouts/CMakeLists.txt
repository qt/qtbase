# Generated from nestedlayouts.pro.

cmake_minimum_required(VERSION 3.14)
project(nestedlayouts LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(nestedlayouts WIN32 MACOSX_BUNDLE
    main.cpp
)
target_link_libraries(nestedlayouts PUBLIC
    Qt::Widgets
)

install(TARGETS nestedlayouts
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
