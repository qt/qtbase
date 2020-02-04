# Generated from fridgemagnets.pro.

cmake_minimum_required(VERSION 3.14)
project(fridgemagnets LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples/widgets/draganddrop/fridgemagnets")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(fridgemagnets
    draglabel.cpp draglabel.h
    dragwidget.cpp dragwidget.h
    main.cpp
)
target_link_libraries(fridgemagnets PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(fridgemagnets_resource_files
    "words.txt"
)

qt6_add_resources(fridgemagnets "fridgemagnets"
    PREFIX
        "/dictionary"
    FILES
        ${fridgemagnets_resource_files}
)

install(TARGETS fridgemagnets
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
