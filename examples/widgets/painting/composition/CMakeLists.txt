# Generated from composition.pro.

cmake_minimum_required(VERSION 3.14)
project(composition LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(composition
    # special case: remove files from ../shared
    composition.cpp composition.h composition.qrc
    main.cpp
)
target_include_directories(composition PUBLIC
    ../shared
)
target_link_libraries(composition PUBLIC
    Qt::Widgets
    painting_shared # special case
)

install(TARGETS composition
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
