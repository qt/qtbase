# Generated from regularexpression.pro.

cmake_minimum_required(VERSION 3.14)
project(regularexpression LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(regularexpression WIN32 MACOSX_BUNDLE
    main.cpp
    regularexpression.qrc
    regularexpressiondialog.cpp regularexpressiondialog.h
)
target_link_libraries(regularexpression PUBLIC
    Qt::Widgets
)

install(TARGETS regularexpression
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
