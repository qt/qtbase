# Generated from regexp.pro.

cmake_minimum_required(VERSION 3.14)
project(regexp LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt5 COMPONENTS Widgets)

add_executable(regexp WIN32 MACOSX_BUNDLE
    main.cpp
    regexpdialog.cpp regexpdialog.h
)
target_link_libraries(regexp PUBLIC
    Qt::Widgets
)

install(TARGETS regexp
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
)
