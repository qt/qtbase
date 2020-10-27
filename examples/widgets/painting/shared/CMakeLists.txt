# special case skip regeneration
# Port of shared.pri to be included by painting examples

if(NOT TARGET Qt::Widgets)
    find_package(Qt6 REQUIRED COMPONENTS Widgets)
endif()

if(NOT TARGET Qt::OpenGL)
    find_package(Qt6 OPTIONAL_COMPONENTS OpenGL)
endif()

add_library(painting_shared OBJECT)
add_library(painting_shared::painting_shared ALIAS painting_shared)
qt6_wrap_cpp(moc_files arthurwidgets.h hoverpoints.h) # no automoc for OBJECT libs:-/
target_sources(painting_shared PRIVATE
    arthurstyle.cpp arthurstyle.h
    arthurwidgets.cpp arthurwidgets.h
    hoverpoints.cpp hoverpoints.h
    ${moc_files}
)
target_link_libraries(painting_shared PUBLIC Qt::Widgets)
target_include_directories(painting_shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

## Scopes:
#####################################################################

if (TARGET Qt::OpenGL OR QT_FEATURE_opengles2)
    target_compile_definitions(painting_shared PRIVATE QT_OPENGL_SUPPORT)
    target_link_libraries(painting_shared PUBLIC
        Qt::OpenGL
    )
    qt6_wrap_cpp(moc_files_gl fbopaintdevice.h) # no automoc for OBJECT libs
    target_sources(painting_shared PRIVATE fbopaintdevice.cpp fbopaintdevice.h ${moc_files_gl})
endif()
