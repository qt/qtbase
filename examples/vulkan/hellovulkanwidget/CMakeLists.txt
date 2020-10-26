# Generated from hellovulkanwidget.pro.

cmake_minimum_required(VERSION 3.14)
project(hellovulkanwidget LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/vulkan/hellovulkanwidget")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(hellovulkanwidget
    ../shared/trianglerenderer.cpp ../shared/trianglerenderer.h
    hellovulkanwidget.cpp hellovulkanwidget.h
    main.cpp
)
set_target_properties(hellovulkanwidget PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_link_libraries(hellovulkanwidget PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set_source_files_properties("../shared/color_frag.spv"
    PROPERTIES QT_RESOURCE_ALIAS "color_frag.spv"
)
set_source_files_properties("../shared/color_vert.spv"
    PROPERTIES QT_RESOURCE_ALIAS "color_vert.spv"
)
set(hellovulkanwidget_resource_files
    "../shared/color_frag.spv"
    "../shared/color_vert.spv"
)

qt6_add_resources(hellovulkanwidget "hellovulkanwidget"
    PREFIX
        "/"
    FILES
        ${hellovulkanwidget_resource_files}
)

install(TARGETS hellovulkanwidget
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
