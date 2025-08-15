# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

add_library(corelib_snippets OBJECT
    qrangemodel/main.cpp
)

target_link_libraries(corelib_snippets PRIVATE
    Qt::Core
)

if(QT_FEATURE_widgets)
    target_link_libraries(corelib_snippets PRIVATE
        Qt::Widgets
    )
endif()

if ("${CMAKE_CXX_COMPILE_FEATURES}" MATCHES "cxx_std_23")
    set_property(TARGET corelib_snippets PROPERTY CXX_STANDARD 23)
endif()

set_target_properties(corelib_snippets PROPERTIES UNITY_BUILD OFF)
