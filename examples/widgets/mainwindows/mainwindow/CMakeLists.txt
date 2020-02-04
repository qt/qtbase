# Generated from mainwindow.pro.

cmake_minimum_required(VERSION 3.14)
project(mainwindow LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/widgets/mainwindows/mainwindow")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(mainwindow
    colorswatch.cpp colorswatch.h
    main.cpp
    mainwindow.cpp mainwindow.h
    toolbar.cpp toolbar.h
)
target_link_libraries(mainwindow PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(mainwindow_resource_files
    "qt.png"
    "titlebarCenter.png"
    "titlebarLeft.png"
    "titlebarRight.png"
)

qt6_add_resources(mainwindow "mainwindow"
    PREFIX
        "/res"
    FILES
        ${mainwindow_resource_files}
)

install(TARGETS mainwindow
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
