# special case: Entire file!

# special case:
add_library(painting_shared OBJECT)
qt6_wrap_cpp(moc_files arthurwidgets.h hoverpoints.h) # no automoc for OBJECT libs:-/
target_sources(painting_shared PRIVATE
    arthurstyle.cpp arthurstyle.h
    arthurwidgets.cpp arthurwidgets.h
    hoverpoints.cpp hoverpoints.h
    ${moc_files}
)
target_link_libraries(painting_shared PUBLIC Qt::Widgets)
target_include_directories(painting_shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

qt6_add_resources(res_files shared.qrc)
target_sources(painting_shared PRIVATE ${res_files})

## Scopes:
#####################################################################

if (TARGET Qt::OpenGL OR QT_FEATURE_opengles2)
    target_compile_definitions(painting_shared PRIVATE QT_OPENGL_SUPPORT)
    target_link_libraries(painting_shared PRIVATE
        Qt::OpenGL
        Qt::Widgets
    )
endif()
