# Generated from licensewizard.pro.

cmake_minimum_required(VERSION 3.14)
project(licensewizard LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS PrintSupport)

add_qt_gui_executable(licensewizard
    licensewizard.cpp licensewizard.h licensewizard.qrc
    main.cpp
)
target_link_libraries(licensewizard PUBLIC
    Qt::PrintSupport
    Qt::Widgets
)

install(TARGETS licensewizard
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
