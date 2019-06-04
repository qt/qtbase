# Generated from books.pro.

cmake_minimum_required(VERSION 3.14)
project(books LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Sql)
find_package(Qt5 COMPONENTS Widgets)

add_executable(books WIN32 MACOSX_BUNDLE
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
)
