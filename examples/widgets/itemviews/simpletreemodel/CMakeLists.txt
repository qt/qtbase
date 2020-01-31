# Generated from simpletreemodel.pro.

cmake_minimum_required(VERSION 3.14)
project(simpletreemodel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(simpletreemodel
    main.cpp
    simpletreemodel.qrc
    treeitem.cpp treeitem.h
    treemodel.cpp treemodel.h
)
target_link_libraries(simpletreemodel PUBLIC
    Qt::Widgets
)

install(TARGETS simpletreemodel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
