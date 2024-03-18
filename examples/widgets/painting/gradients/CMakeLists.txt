# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(gradients LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets)

qt_standard_project_setup()

qt_add_executable(gradients
    gradients.cpp gradients.h
    main.cpp
)

set_target_properties(gradients PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
)

if(NOT TARGET painting_shared::painting_shared)
    include(../shared/use_lib.cmake)
endif()

target_link_libraries(gradients PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
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

qt_add_resources(gradients "shared"
    PREFIX
        "/res"
    BASE
        "../shared"
    FILES
        ${shared_resource_files}
)

set(gradients_resource_files
    "gradients.cpp"
    "gradients.html"
)

qt_add_resources(gradients "gradients"
    PREFIX
        "/res/gradients"
    FILES
        ${gradients_resource_files}
)

install(TARGETS gradients
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_app_script(
    TARGET gradients
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
