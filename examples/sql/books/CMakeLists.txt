# Generated from books.pro.

cmake_minimum_required(VERSION 3.14)
project(books LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Sql)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(books
    bookdelegate.cpp bookdelegate.h
    books.qrc
    bookwindow.cpp bookwindow.h bookwindow.ui
    initdb.h
    main.cpp
)
target_include_directories(books PUBLIC
    .
)
target_link_libraries(books PUBLIC
    Qt::Sql
    Qt::Widgets
)

install(TARGETS books
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
