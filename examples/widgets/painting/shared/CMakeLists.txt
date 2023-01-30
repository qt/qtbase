# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_library(painting_shared OBJECT)
add_library(painting_shared::painting_shared ALIAS painting_shared)
qt6_wrap_cpp(moc_files arthurwidgets.h hoverpoints.h) # no automoc for OBJECT libs:-/
target_sources(painting_shared PRIVATE
    arthurstyle.cpp arthurstyle.h
    arthurwidgets.cpp arthurwidgets.h
    hoverpoints.cpp hoverpoints.h
    ${moc_files}
)

set_target_properties(painting_shared PROPERTIES UNITY_BUILD OFF)

target_link_libraries(painting_shared PUBLIC Qt6::Widgets)
target_include_directories(painting_shared PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}")

## Scopes:
#####################################################################

if (TARGET Qt6::OpenGL OR QT_FEATURE_opengles2)
    target_compile_definitions(painting_shared PRIVATE QT_OPENGL_SUPPORT)
    target_link_libraries(painting_shared PUBLIC
        Qt6::OpenGL
    )
    qt6_wrap_cpp(moc_files_gl fbopaintdevice.h) # no automoc for OBJECT libs
    target_sources(painting_shared PRIVATE fbopaintdevice.cpp fbopaintdevice.h ${moc_files_gl})
endif()
