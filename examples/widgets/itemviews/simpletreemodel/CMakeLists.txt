# Generated from simpletreemodel.pro.

cmake_minimum_required(VERSION 3.14)
project(simpletreemodel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/itemviews/simpletreemodel")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(simpletreemodel
    main.cpp
    treeitem.cpp treeitem.h
    treemodel.cpp treemodel.h
)
set_target_properties(simpletreemodel PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(simpletreemodel PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(simpletreemodel_resource_files
    "default.txt"
)

qt6_add_resources(simpletreemodel "simpletreemodel"
    PREFIX
        "/"
    FILES
        ${simpletreemodel_resource_files}
)

install(TARGETS simpletreemodel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
