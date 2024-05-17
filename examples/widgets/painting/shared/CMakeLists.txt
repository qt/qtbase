# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

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
