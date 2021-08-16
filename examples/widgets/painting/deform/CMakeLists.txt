# Generated from deform.pro.

cmake_minimum_required(VERSION 3.16)
project(deform LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/painting/deform")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(deform
    # special case begin
    # remove files from ../shared
    #../shared/arthurstyle.cpp ../shared/arthurstyle.h
    #../shared/arthurwidgets.cpp ../shared/arthurwidgets.h
    #../shared/hoverpoints.cpp ../shared/hoverpoints.h
    # special case end
    main.cpp
    pathdeform.cpp pathdeform.h
)
set_target_properties(deform PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_include_directories(deform PUBLIC
    ../shared
)

# special case begin
if(NOT TARGET painting_shared::painting_shared)
    include(../shared/use_lib.cmake)
endif()
# special case end

target_link_libraries(deform PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
    painting_shared::painting_shared # special case
)


# Resources:
set(shared_resource_files
    "../shared/images/button_normal_cap_left.png"
    "../shared/images/button_normal_cap_right.png"
    "../shared/images/button_normal_stretch.png"
    "../shared/images/button_pressed_cap_left.png"
    "../shared/images/button_pressed_cap_right.png"
    "../shared/images/button_pressed_stretch.png"
    "../shared/images/frame_bottom.png"
    "../shared/images/frame_bottomleft.png"
    "../shared/images/frame_bottomright.png"
    "../shared/images/frame_left.png"
    "../shared/images/frame_right.png"
    "../shared/images/frame_top.png"
    "../shared/images/frame_topleft.png"
    "../shared/images/frame_topright.png"
    "../shared/images/groupframe_bottom_left.png"
    "../shared/images/groupframe_bottom_right.png"
    "../shared/images/groupframe_bottom_stretch.png"
    "../shared/images/groupframe_left_stretch.png"
    "../shared/images/groupframe_right_stretch.png"
    "../shared/images/groupframe_top_stretch.png"
    "../shared/images/groupframe_topleft.png"
    "../shared/images/groupframe_topright.png"
    "../shared/images/line_dash_dot.png"
    "../shared/images/line_dash_dot_dot.png"
    "../shared/images/line_dashed.png"
    "../shared/images/line_dotted.png"
    "../shared/images/line_solid.png"
    "../shared/images/radiobutton-on.png"
    "../shared/images/radiobutton_off.png"
    "../shared/images/radiobutton_on.png"
    "../shared/images/slider_bar.png"
    "../shared/images/slider_thumb_on.png"
    "../shared/images/title_cap_left.png"
    "../shared/images/title_cap_right.png"
    "../shared/images/title_stretch.png"
)

qt6_add_resources(deform "shared"
    PREFIX
        "/res"
    BASE
        "../shared"
    FILES
        ${shared_resource_files}
)
set(deform_resource_files
    "pathdeform.cpp"
    "pathdeform.html"
)

qt6_add_resources(deform "deform"
    PREFIX
        "/res/deform"
    FILES
        ${deform_resource_files}
)

# special case begin
# remove files from ../shared
#if(QT_FEATURE_opengl)
    #target_sources(affine PUBLIC
        #../shared/fbopaintdevice.cpp ../shared/fbopaintdevice.h
    #)

    #target_link_libraries(affine PUBLIC
        #Qt::OpenGL
    #)
#endif()
# special case end

install(TARGETS deform
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
