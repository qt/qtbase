# Generated from saxbookmarks.pro.

cmake_minimum_required(VERSION 3.14)
project(saxbookmarks LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Xml)
find_package(Qt5 COMPONENTS Widgets)

add_executable(saxbookmarks WIN32 MACOSX_BUNDLE
    main.cpp
    mainwindow.cpp mainwindow.h
    xbelgenerator.cpp xbelgenerator.h
    xbelhandler.cpp xbelhandler.h
)
target_link_libraries(saxbookmarks PUBLIC
    Qt::Widgets
    Qt::Xml
)
install(TARGETS saxbookmarks
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
