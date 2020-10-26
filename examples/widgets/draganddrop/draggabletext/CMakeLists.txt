# Generated from draggabletext.pro.

cmake_minimum_required(VERSION 3.14)
project(draggabletext LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/draganddrop/draggabletext")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(draggabletext
    dragwidget.cpp dragwidget.h
    main.cpp
)
set_target_properties(draggabletext PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(draggabletext PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(draggabletext_resource_files
    "words.txt"
)

qt6_add_resources(draggabletext "draggabletext"
    PREFIX
        "/dictionary"
    FILES
        ${draggabletext_resource_files}
)

install(TARGETS draggabletext
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
