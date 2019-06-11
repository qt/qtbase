# Generated from codecs.pro.

cmake_minimum_required(VERSION 3.14)
project(codecs LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_qt_gui_executable(codecs WIN32 MACOSX_BUNDLE
    codecs.qrc
    encodingdialog.cpp encodingdialog.h
    main.cpp
    mainwindow.cpp mainwindow.h
    previewform.cpp previewform.h
)
target_link_libraries(codecs PUBLIC
    Qt::Widgets
)

install(TARGETS codecs
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
