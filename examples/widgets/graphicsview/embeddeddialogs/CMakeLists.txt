# Generated from embeddeddialogs.pro.

cmake_minimum_required(VERSION 3.14)
project(embeddeddialogs LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(embeddeddialogs WIN32 MACOSX_BUNDLE
    customproxy.cpp customproxy.h
    embeddeddialog.cpp embeddeddialog.h embeddeddialog.ui
    embeddeddialogs.qrc
    main.cpp
)
target_link_libraries(embeddeddialogs PUBLIC
    Qt::Widgets
)

install(TARGETS embeddeddialogs
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
