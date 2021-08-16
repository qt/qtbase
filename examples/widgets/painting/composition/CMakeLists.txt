# Generated from composition.pro.

cmake_minimum_required(VERSION 3.16)
project(composition LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
  set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/painting/composition")

find_package(Qt6 COMPONENTS Core)
find_package(Qt6 COMPONENTS Gui)
find_package(Qt6 COMPONENTS Widgets)

qt_add_executable(composition
    # special case begin
    # remove files from ../shared
    #../shared/arthurstyle.cpp ../shared/arthurstyle.h
    #../shared/arthurwidgets.cpp ../shared/arthurwidgets.h
    #../shared/hoverpoints.cpp ../shared/hoverpoints.h
    # special case end
    composition.cpp composition.h
    main.cpp
)
set_target_properties(composition PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)
target_include_directories(composition PUBLIC
    ../shared
)

# special case begin
if(NOT TARGET painting_shared::painting_shared)
    include(../shared/use_lib.cmake)
endif()
# special case end

target_link_libraries(composition PUBLIC
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

qt6_add_resources(composition "shared"
    PREFIX
        "/res"
    BASE
        "../shared"
    FILES
        ${shared_resource_files}
)
set(composition_resource_files
    "composition.cpp"
    "composition.html"
    "flower.jpg"
    "flower_alpha.jpg"
)

qt6_add_resources(composition "composition"
    PREFIX
        "/res/composition"
    FILES
        ${composition_resource_files}
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

install(TARGETS composition
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
