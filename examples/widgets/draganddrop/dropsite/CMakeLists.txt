# Generated from dropsite.pro.

cmake_minimum_required(VERSION 3.14)
project(dropsite LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/widgets/draganddrop/dropsite")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(dropsite
    droparea.cpp droparea.h
    dropsitewindow.cpp dropsitewindow.h
    main.cpp
)
target_link_libraries(dropsite PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS dropsite
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
