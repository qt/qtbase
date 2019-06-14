# Generated from fingerpaint.pro.

cmake_minimum_required(VERSION 3.14)
project(fingerpaint LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS PrintSupport) # special case

add_qt_gui_executable(fingerpaint
    main.cpp
    mainwindow.cpp mainwindow.h
    scribblearea.cpp scribblearea.h
)
target_link_libraries(fingerpaint PUBLIC
    Qt::Widgets
)
# special case begin
if (TARGET Qt::PrintSupport)
    target_link_libraries(fingerpaint PUBLIC Qt::PrintSupport)
endif()
# special case end

install(TARGETS fingerpaint
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
