# Generated from basictools.pro.

cmake_minimum_required(VERSION 3.14)
project(pnp_basictools LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/tools/plugandpaint/plugins")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_plugin(pnp_basictools STATIC)
target_sources(pnp_basictools PRIVATE
    basictoolsplugin.cpp basictoolsplugin.h
)
set_target_properties(pnp_basictools PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_include_directories(pnp_basictools PUBLIC
    ../../app
)

target_link_libraries(pnp_basictools PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)

install(TARGETS pnp_basictools
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
