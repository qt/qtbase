# Generated from imageviewer.pro.

cmake_minimum_required(VERSION 3.14)
project(imageviewer LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS PrintSupport) # special case

add_qt_gui_executable(imageviewer
    imageviewer.cpp imageviewer.h
    main.cpp
)
target_link_libraries(imageviewer PUBLIC
    Qt::Widgets
)
# special case begin
if (TARGET Qt::PrintSupport)
    target_link_libraries(imageviewer PUBLIC Qt::PrintSupport)
endif()
# special case end

install(TARGETS imageviewer
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
