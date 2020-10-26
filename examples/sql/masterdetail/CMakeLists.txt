# Generated from masterdetail.pro.

cmake_minimum_required(VERSION 3.14)
project(masterdetail LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/sql/masterdetail")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Sql)
find_package(Qt6 COMPONENTS Widgets)
find_package(Qt6 COMPONENTS Xml)

qt_add_executable(masterdetail
    database.h
    dialog.cpp dialog.h
    main.cpp
    mainwindow.cpp mainwindow.h
)
set_target_properties(masterdetail PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(masterdetail PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Sql
    Qt::Widgets
    Qt::Xml
)


# Resources:
set(masterdetail_resource_files
    "images/icon.png"
    "images/image.png"
)

qt6_add_resources(masterdetail "masterdetail"
    PREFIX
        "/"
    FILES
        ${masterdetail_resource_files}
)

install(TARGETS masterdetail
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
