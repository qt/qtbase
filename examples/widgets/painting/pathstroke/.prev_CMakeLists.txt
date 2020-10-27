# Generated from pathstroke.pro.

cmake_minimum_required(VERSION 3.14)
project(pathstroke LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/painting/pathstroke")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(pathstroke
    ../shared/arthurstyle.cpp ../shared/arthurstyle.h
    ../shared/arthurwidgets.cpp ../shared/arthurwidgets.h
    ../shared/hoverpoints.cpp ../shared/hoverpoints.h
    main.cpp
    pathstroke.cpp pathstroke.h
)
set_target_properties(pathstroke PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_include_directories(pathstroke PUBLIC
    ../shared
)

target_link_libraries(pathstroke PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
)


# Resources:
set_source_files_properties("../shared/images/button_normal_cap_left.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/button_normal_cap_left.png"
)
set_source_files_properties("../shared/images/button_normal_cap_right.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/button_normal_cap_right.png"
)
set_source_files_properties("../shared/images/button_normal_stretch.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/button_normal_stretch.png"
)
set_source_files_properties("../shared/images/button_pressed_cap_left.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/button_pressed_cap_left.png"
)
set_source_files_properties("../shared/images/button_pressed_cap_right.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/button_pressed_cap_right.png"
)
set_source_files_properties("../shared/images/button_pressed_stretch.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/button_pressed_stretch.png"
)
set_source_files_properties("../shared/images/frame_bottom.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/frame_bottom.png"
)
set_source_files_properties("../shared/images/frame_bottomleft.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/frame_bottomleft.png"
)
set_source_files_properties("../shared/images/frame_bottomright.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/frame_bottomright.png"
)
set_source_files_properties("../shared/images/frame_left.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/frame_left.png"
)
set_source_files_properties("../shared/images/frame_right.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/frame_right.png"
)
set_source_files_properties("../shared/images/frame_top.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/frame_top.png"
)
set_source_files_properties("../shared/images/frame_topleft.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/frame_topleft.png"
)
set_source_files_properties("../shared/images/frame_topright.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/frame_topright.png"
)
set_source_files_properties("../shared/images/groupframe_bottom_left.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/groupframe_bottom_left.png"
)
set_source_files_properties("../shared/images/groupframe_bottom_right.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/groupframe_bottom_right.png"
)
set_source_files_properties("../shared/images/groupframe_bottom_stretch.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/groupframe_bottom_stretch.png"
)
set_source_files_properties("../shared/images/groupframe_left_stretch.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/groupframe_left_stretch.png"
)
set_source_files_properties("../shared/images/groupframe_right_stretch.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/groupframe_right_stretch.png"
)
set_source_files_properties("../shared/images/groupframe_top_stretch.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/groupframe_top_stretch.png"
)
set_source_files_properties("../shared/images/groupframe_topleft.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/groupframe_topleft.png"
)
set_source_files_properties("../shared/images/groupframe_topright.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/groupframe_topright.png"
)
set_source_files_properties("../shared/images/line_dash_dot.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/line_dash_dot.png"
)
set_source_files_properties("../shared/images/line_dash_dot_dot.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/line_dash_dot_dot.png"
)
set_source_files_properties("../shared/images/line_dashed.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/line_dashed.png"
)
set_source_files_properties("../shared/images/line_dotted.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/line_dotted.png"
)
set_source_files_properties("../shared/images/line_solid.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/line_solid.png"
)
set_source_files_properties("../shared/images/radiobutton-on.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/radiobutton-on.png"
)
set_source_files_properties("../shared/images/radiobutton_off.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/radiobutton_off.png"
)
set_source_files_properties("../shared/images/radiobutton_on.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/radiobutton_on.png"
)
set_source_files_properties("../shared/images/slider_bar.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/slider_bar.png"
)
set_source_files_properties("../shared/images/slider_thumb_on.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/slider_thumb_on.png"
)
set_source_files_properties("../shared/images/title_cap_left.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/title_cap_left.png"
)
set_source_files_properties("../shared/images/title_cap_right.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/title_cap_right.png"
)
set_source_files_properties("../shared/images/title_stretch.png"
    PROPERTIES QT_RESOURCE_ALIAS "images/title_stretch.png"
)
set(shared_resource_files
    "images/button_normal_cap_left.png"
    "images/button_normal_cap_right.png"
    "images/button_normal_stretch.png"
    "images/button_pressed_cap_left.png"
    "images/button_pressed_cap_right.png"
    "images/button_pressed_stretch.png"
    "images/frame_bottom.png"
    "images/frame_bottomleft.png"
    "images/frame_bottomright.png"
    "images/frame_left.png"
    "images/frame_right.png"
    "images/frame_top.png"
    "images/frame_topleft.png"
    "images/frame_topright.png"
    "images/groupframe_bottom_left.png"
    "images/groupframe_bottom_right.png"
    "images/groupframe_bottom_stretch.png"
    "images/groupframe_left_stretch.png"
    "images/groupframe_right_stretch.png"
    "images/groupframe_top_stretch.png"
    "images/groupframe_topleft.png"
    "images/groupframe_topright.png"
    "images/line_dash_dot.png"
    "images/line_dash_dot_dot.png"
    "images/line_dashed.png"
    "images/line_dotted.png"
    "images/line_solid.png"
    "images/radiobutton-on.png"
    "images/radiobutton_off.png"
    "images/radiobutton_on.png"
    "images/slider_bar.png"
    "images/slider_thumb_on.png"
    "images/title_cap_left.png"
    "images/title_cap_right.png"
    "images/title_stretch.png"
)

qt6_add_resources(pathstroke "shared"
    PREFIX
        "/res"
    BASE
        "../shared"
    FILES
        ${shared_resource_files}
)
set(pathstroke_resource_files
    "pathstroke.cpp"
    "pathstroke.html"
)

qt6_add_resources(pathstroke "pathstroke"
    PREFIX
        "/res/pathstroke"
    FILES
        ${pathstroke_resource_files}
)

if(QT_FEATURE_opengl)
    target_sources(pathstroke PUBLIC
        ../shared/fbopaintdevice.cpp ../shared/fbopaintdevice.h
    )

    target_link_libraries(pathstroke PUBLIC
        Qt::OpenGL
    )
endif()

install(TARGETS pathstroke
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
