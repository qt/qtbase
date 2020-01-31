# Generated from padnavigator.pro.

cmake_minimum_required(VERSION 3.14)
project(padnavigator LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

set(INSTALL_EXAMPLEDIR "examples")

find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS OpenGL) # special case

add_executable(padnavigator
    flippablepad.cpp flippablepad.h
    form.ui
    main.cpp
    padnavigator.cpp padnavigator.h padnavigator.qrc
    roundrectitem.cpp roundrectitem.h
    splashitem.cpp splashitem.h
)
target_link_libraries(padnavigator PUBLIC
    Qt::Widgets
)
# special case begin
if (TARGET Qt::OpenGL)
    target_link_libraries(padnavigator PUBLIC Qt::OpenGL)
endif()
# special case end

install(TARGETS padnavigator
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
