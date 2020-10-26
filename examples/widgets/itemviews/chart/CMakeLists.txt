# Generated from chart.pro.

cmake_minimum_required(VERSION 3.14)
project(chart LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/itemviews/chart")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(chart
    main.cpp
    mainwindow.cpp mainwindow.h
    pieview.cpp pieview.h
)
set_target_properties(chart PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(chart PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set(chart_resource_files
    "qtdata.cht"
)

qt6_add_resources(chart "chart"
    PREFIX
        "/Charts"
    FILES
        ${chart_resource_files}
)

if(UNIX AND NOT APPLE AND NOT HAIKU AND NOT INTEGRITY AND NOT VXWORKS)
    target_link_libraries(chart PUBLIC
        m
    )
endif()

install(TARGETS chart
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
