# Generated from undoframework.pro.

cmake_minimum_required(VERSION 3.14)
project(undoframework LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/undoframework")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(undoframework
    commands.cpp commands.h
    diagramitem.cpp diagramitem.h
    diagramscene.cpp diagramscene.h
    main.cpp
    mainwindow.cpp mainwindow.h
)
set_target_properties(undoframework PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(undoframework PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(undoframework_resource_files
    "images/cross.png"
)

qt6_add_resources(undoframework "undoframework"
    PREFIX
        "/"
    FILES
        ${undoframework_resource_files}
)

install(TARGETS undoframework
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
