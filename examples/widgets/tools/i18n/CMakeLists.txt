# Generated from i18n.pro.

cmake_minimum_required(VERSION 3.14)
project(i18n LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(i18n WIN32 MACOSX_BUNDLE
    i18n.qrc
    languagechooser.cpp languagechooser.h
    main.cpp
    mainwindow.cpp mainwindow.h
)
target_link_libraries(i18n PUBLIC
    Qt::Widgets
)

install(TARGETS i18n
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
