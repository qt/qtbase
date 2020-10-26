# Generated from simpledommodel.pro.

cmake_minimum_required(VERSION 3.14)
project(simpledommodel LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/itemviews/simpledommodel")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Xml)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(simpledommodel
    domitem.cpp domitem.h
    dommodel.cpp dommodel.h
    main.cpp
    mainwindow.cpp mainwindow.h
)
set_target_properties(simpledommodel PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(simpledommodel PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
    Qt::Xml
)

install(TARGETS simpledommodel
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
