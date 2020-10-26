# Generated from books.pro.

cmake_minimum_required(VERSION 3.14)
project(books LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/sql/books")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Sql)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(books
    bookdelegate.cpp bookdelegate.h
    bookwindow.cpp bookwindow.h bookwindow.ui
    initdb.h
    main.cpp
)
set_target_properties(books PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_include_directories(books PUBLIC
    .
)

target_link_libraries(books PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Sql
    Qt::Widgets
)


# Resources:
set(books_resource_files
    "images/star.png"
)

qt6_add_resources(books "books"
    PREFIX
        "/"
    FILES
        ${books_resource_files}
)

install(TARGETS books
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
