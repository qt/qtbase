# Generated from customcompleter.pro.

cmake_minimum_required(VERSION 3.14)
project(customcompleter LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(customcompleter WIN32 MACOSX_BUNDLE
    customcompleter.qrc
    main.cpp
    mainwindow.cpp mainwindow.h
    textedit.cpp textedit.h
)
target_link_libraries(customcompleter PUBLIC
    Qt::Widgets
)

install(TARGETS customcompleter
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
