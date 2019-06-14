# Generated from settingseditor.pro.

cmake_minimum_required(VERSION 3.14)
project(settingseditor LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(settingseditor
    locationdialog.cpp locationdialog.h
    main.cpp
    mainwindow.cpp mainwindow.h
    settingstree.cpp settingstree.h
    variantdelegate.cpp variantdelegate.h
)
target_link_libraries(settingseditor PUBLIC
    Qt::Widgets
)

install(TARGETS settingseditor
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
