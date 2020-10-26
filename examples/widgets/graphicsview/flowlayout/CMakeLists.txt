# Generated from flowlayout.pro.

cmake_minimum_required(VERSION 3.14)
project(flowlayout LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/graphicsview_flowlayout") # special case

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(graphicsview_flowlayout # special case: renamed flowlayout
    flowlayout.cpp flowlayout.h
    main.cpp
    window.cpp window.h
)
set_target_properties(graphicsview_flowlayout PROPERTIES # special case
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(graphicsview_flowlayout PUBLIC # special case
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS graphicsview_flowlayout # special case: renamed flowlayout
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
