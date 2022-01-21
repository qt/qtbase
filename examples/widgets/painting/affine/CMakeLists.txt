cmake_minimum_required(VERSION 3.16)
project(affine LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/widgets/painting/affine")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_add_executable(affine
    # remove files from ../shared
    #../shared/arthurstyle.cpp ../shared/arthurstyle.h
    #../shared/arthurwidgets.cpp ../shared/arthurwidgets.h
    #../shared/hoverpoints.cpp ../shared/hoverpoints.h
    main.cpp
    xform.cpp xform.h
)

set_target_properties(affine PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

target_include_directories(affine PUBLIC
    ../shared
)

if(NOT TARGET painting_shared::painting_shared)
    include(../shared/use_lib.cmake)
endif()

target_link_libraries(affine PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Widgets
    painting_shared::painting_shared
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

qt_add_resources(affine "shared"
    PREFIX
        "/res"
    BASE
        "../shared"
    FILES
        ${shared_resource_files}
)

set(affine_resource_files
    "bg1.jpg"
    "xform.cpp"
    "xform.html"
)

qt_add_resources(affine "affine"
    PREFIX
        "/res/affine"
    FILES
        ${affine_resource_files}
)

# remove files from ../shared
#if(QT_FEATURE_opengl)
    #target_sources(affine PUBLIC
        #../shared/fbopaintdevice.cpp ../shared/fbopaintdevice.h
    #)

    #target_link_libraries(affine PUBLIC
        #Qt::OpenGL
    #)
#endif()

install(TARGETS affine
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
