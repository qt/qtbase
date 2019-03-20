# special case: Entire file!

# special case:
add_library(painting_shared OBJECT)
qt_manual_moc(moc_files arthurwidgets.h hoverpoints.h) # no automoc for OBJECT libs:-/
target_sources(painting_shared PRIVATE
    arthurstyle.cpp arthurstyle.h
    arthurwidgets.cpp arthurwidgets.h
    hoverpoints.cpp hoverpoints.h
    ${moc_files}
)
target_link_libraries(painting_shared PUBLIC Qt::Widgets)
target_include_directories(painting_shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")


# Resources:
add_qt_resource(painting_shared "shared" PREFIX "/res" FILES
    images/button_normal_cap_left.png
    images/button_normal_cap_right.png
    images/button_normal_stretch.png
    images/button_pressed_cap_left.png
    images/button_pressed_cap_right.png
    images/button_pressed_stretch.png
    images/frame_bottom.png
    images/frame_bottomleft.png
    images/frame_bottomright.png
    images/frame_left.png
    images/frame_right.png
    images/frame_top.png
    images/frame_topleft.png
    images/frame_topright.png
    images/groupframe_bottom_left.png
    images/groupframe_bottom_right.png
    images/groupframe_bottom_stretch.png
    images/groupframe_left_stretch.png
    images/groupframe_right_stretch.png
    images/groupframe_top_stretch.png
    images/groupframe_topleft.png
    images/groupframe_topright.png
    images/line_dash_dot.png
    images/line_dash_dot_dot.png
    images/line_dashed.png
    images/line_dotted.png
    images/line_solid.png
    images/radiobutton-on.png
    images/radiobutton_off.png
    images/radiobutton_on.png
    images/slider_bar.png
    images/slider_thumb_on.png
    images/title_cap_left.png
    images/title_cap_right.png
    images/title_stretch.png)


## Scopes:
#####################################################################

extend_target(painting_shared CONDITION TARGET Qt::OpenGL OR QT_FEATURE_opengles2
    DEFINES
        QT_OPENGL_SUPPORT
    LIBRARIES
        Qt::OpenGL
        Qt::Widgets
)
