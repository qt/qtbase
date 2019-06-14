# Generated from customsortfiltermodel.pro.

cmake_minimum_required(VERSION 3.14)
project(customsortfiltermodel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(customsortfiltermodel
    customsortfiltermodel.qrc
    filterwidget.cpp filterwidget.h
    main.cpp
    mysortfilterproxymodel.cpp mysortfilterproxymodel.h
    window.cpp window.h
)
target_link_libraries(customsortfiltermodel PUBLIC
    Qt::Widgets
)

install(TARGETS customsortfiltermodel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
