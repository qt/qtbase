# Generated from drilldown.pro.

cmake_minimum_required(VERSION 3.14)
project(drilldown LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Sql)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(drilldown
    ../connection.h
    drilldown.qrc
    imageitem.cpp imageitem.h
    informationwindow.cpp informationwindow.h
    main.cpp
    view.cpp view.h
)
target_link_libraries(drilldown PUBLIC
    Qt::Sql
    Qt::Widgets
)

install(TARGETS drilldown
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
