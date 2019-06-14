# Generated from treemodelcompleter.pro.

cmake_minimum_required(VERSION 3.14)
project(treemodelcompleter LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(treemodelcompleter
    main.cpp
    mainwindow.cpp mainwindow.h
    treemodelcompleter.cpp treemodelcompleter.h treemodelcompleter.qrc
)
target_link_libraries(treemodelcompleter PUBLIC
    Qt::Widgets
)

install(TARGETS treemodelcompleter
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
