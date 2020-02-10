# Generated from mdi.pro.

cmake_minimum_required(VERSION 3.14)
project(mdi LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/widgets/mainwindows/mdi")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(mdi
    main.cpp
    mainwindow.cpp mainwindow.h
    mdichild.cpp mdichild.h
)
target_link_libraries(mdi PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(mdi_resource_files
    "images/copy.png"
    "images/cut.png"
    "images/new.png"
    "images/open.png"
    "images/paste.png"
    "images/save.png"
)

qt6_add_resources(mdi "mdi"
    PREFIX
        "/"
    FILES
        ${mdi_resource_files}
)

install(TARGETS mdi
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
