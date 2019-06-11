# Generated from querymodel.pro.

cmake_minimum_required(VERSION 3.14)
project(querymodel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Sql)
find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(querymodel WIN32 MACOSX_BUNDLE
    ../connection.h
    customsqlmodel.cpp customsqlmodel.h
    editablesqlmodel.cpp editablesqlmodel.h
    main.cpp
)
target_link_libraries(querymodel PUBLIC
    Qt::Sql
    Qt::Widgets
)

install(TARGETS querymodel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
