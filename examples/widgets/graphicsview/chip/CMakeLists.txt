# Generated from chip.pro.

cmake_minimum_required(VERSION 3.14)
project(chip LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS OpenGL) # special case
find_package(Qt6 COMPONENTS PrintSupport) # special case

add_qt_gui_executable(chip
    chip.cpp chip.h
    images.qrc
    main.cpp
    mainwindow.cpp mainwindow.h
    view.cpp view.h
)
target_link_libraries(chip PUBLIC
    Qt::Widgets
)
if (TARGET Qt::OpenGL)
    target_link_libraries(chip PUBLIC Qt::OpenGL)
endif()
if (TARGET Qt::PrintSupport)
    target_link_libraries(chip PUBLIC Qt::PrintSupport)
endif()

install(TARGETS chip
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
