# Generated from qrasterwindow.pro.

cmake_minimum_required(VERSION 3.14)
project(qrasterwindow LANGUAGES CXX)

find_package(Qt6 COMPONENTS Gui) # special case

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

add_qt_gui_executable(qrasterwindow
    main.cpp
)
target_link_libraries(qrasterwindow PUBLIC Qt::Gui) # special case: add

install(TARGETS qrasterwindow
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
