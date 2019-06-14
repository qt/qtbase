# Generated from editabletreemodel.pro.

cmake_minimum_required(VERSION 3.14)
project(editabletreemodel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(editabletreemodel
    editabletreemodel.qrc
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    treeitem.cpp treeitem.h
    treemodel.cpp treemodel.h
)
target_link_libraries(editabletreemodel PUBLIC
    Qt::Widgets
)

install(TARGETS editabletreemodel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
