# Generated from sqlbrowser.pro.

cmake_minimum_required(VERSION 3.14)
project(sqlbrowser LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Sql)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(sqlbrowser
    browser.cpp browser.h
    browserwidget.ui
    connectionwidget.cpp connectionwidget.h
    main.cpp
    qsqlconnectiondialog.cpp qsqlconnectiondialog.h qsqlconnectiondialog.ui
)
target_link_libraries(sqlbrowser PUBLIC
    Qt::Sql
    Qt::Widgets
)

install(TARGETS sqlbrowser
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
