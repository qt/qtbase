project(my_app)
cmake_minimum_required(VERSION 3.16)
find_package(Qt6 REQUIRED COMPONENTS Widgets)

#! [AUTORCC]
set(CMAKE_AUTORCC ON)

qt_add_executable(my_app
    application.qrc
    mainwindow.cpp)
#! [AUTORCC]

#! [qt_add_resources]
qt_add_resources(my_app "app_images"
    PREFIX "/"
    FILES
        "images/copy.png"
        "images/cut.png"
        "images/new.png"
        "images/open.png"
        "images/paste.png"
        "images/save.png")
#! [qt_add_resources]
