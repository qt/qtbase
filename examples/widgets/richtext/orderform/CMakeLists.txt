# Generated from orderform.pro.

cmake_minimum_required(VERSION 3.14)
project(orderform LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/richtext/orderform")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(orderform
    detailsdialog.cpp detailsdialog.h
    main.cpp
    mainwindow.cpp mainwindow.h
)
set_target_properties(orderform PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(orderform PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

if(TARGET Qt::PrintSupport)
    target_link_libraries(orderform PUBLIC
        Qt::PrintSupport
    )
endif()

install(TARGETS orderform
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
