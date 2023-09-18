# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_library(bindableproperties_shared STATIC
    subscriptionwindow.cpp
    subscriptionwindow.h
    subscriptionwindow.ui
)

target_link_libraries(bindableproperties_shared PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
)

qt_add_resources(bindableproperties_shared "countries"
    PREFIX
        "/"
    FILES
        "finland.png"
        "germany.png"
        "norway.png"
)
