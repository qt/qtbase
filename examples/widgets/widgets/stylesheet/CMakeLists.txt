# Generated from stylesheet.pro.

cmake_minimum_required(VERSION 3.14)
project(stylesheet LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(stylesheet
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    stylesheet.qrc
    stylesheeteditor.cpp stylesheeteditor.h stylesheeteditor.ui
)
target_link_libraries(stylesheet PUBLIC
    Qt::Widgets
)

install(TARGETS stylesheet
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
