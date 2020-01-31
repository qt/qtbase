# Generated from icons.pro.

cmake_minimum_required(VERSION 3.14)
project(icons LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)

add_qt_gui_executable(icons
    iconpreviewarea.cpp iconpreviewarea.h
    iconsizespinbox.cpp iconsizespinbox.h
    imagedelegate.cpp imagedelegate.h
    main.cpp
    mainwindow.cpp mainwindow.h
)
target_compile_definitions(icons PUBLIC
    SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
)
target_link_libraries(icons PUBLIC
    Qt::Widgets
)
install(TARGETS icons
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
