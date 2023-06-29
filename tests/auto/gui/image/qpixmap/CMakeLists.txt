# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qpixmap Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qpixmap LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    convertFromImage/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    convertFromToHICON/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    loadFromData/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    images/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qpixmap
    SOURCES
        tst_qpixmap.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
    TESTDATA ${test_data}
)

# Resources:
set(qpixmap_resource_files
    "convertFromImage/task31722_0/img1.png"
    "convertFromImage/task31722_0/img2.png"
    "convertFromImage/task31722_1/img1.png"
    "convertFromImage/task31722_1/img2.png"
    "convertFromToHICON/icon_32bpp.ico"
    "convertFromToHICON/icon_32bpp_16x16.png"
    "convertFromToHICON/icon_32bpp_256x256.png"
    "convertFromToHICON/icon_32bpp_32x32.png"
    "convertFromToHICON/icon_32bpp_48x48.png"
    "convertFromToHICON/icon_8bpp.ico"
    "convertFromToHICON/icon_8bpp_16x16.png"
    "convertFromToHICON/icon_8bpp_32x32.png"
    "convertFromToHICON/icon_8bpp_48x48.png"
    "images/designer.png"
    "images/dx_-10_dy_-10_50_50_100_100.png"
    "images/dx_-10_dy_-10_x_y_w_h.png"
    "images/dx_-10_dy_0_50_50_100_100.png"
    "images/dx_-10_dy_0_x_y_w_h.png"
    "images/dx_-128_dy_-128_x_y_w_h.png"
    "images/dx_-128_dy_0_x_y_w_h.png"
    "images/dx_0_dy_-10_50_50_100_100.png"
    "images/dx_0_dy_-10_x_y_w_h.png"
    "images/dx_0_dy_-128_x_y_w_h.png"
    "images/dx_0_dy_0_50_50_100_100.png"
    "images/dx_0_dy_0_null.png"
    "images/dx_0_dy_0_x_y_w_h.png"
    "images/dx_0_dy_10_50_50_100_100.png"
    "images/dx_0_dy_10_x_y_w_h.png"
    "images/dx_0_dy_128_x_y_w_h.png"
    "images/dx_0_dy_1_null.png"
    "images/dx_10_dy_0_50_50_100_100.png"
    "images/dx_10_dy_0_x_y_w_h.png"
    "images/dx_10_dy_10_50_50_100_100.png"
    "images/dx_10_dy_10_x_y_w_h.png"
    "images/dx_128_dy_0_x_y_w_h.png"
    "images/dx_128_dy_128_64_64_128_128.png"
    "images/dx_128_dy_128_x_y_w_h.png"
    "images/dx_1_dy_0_null.png"
    "loadFromData/designer_argb32.png"
    "loadFromData/designer_indexed8_no_alpha.gif"
    "loadFromData/designer_indexed8_no_alpha.png"
    "loadFromData/designer_indexed8_no_alpha_animated.gif"
    "loadFromData/designer_indexed8_with_alpha.gif"
    "loadFromData/designer_indexed8_with_alpha.png"
    "loadFromData/designer_indexed8_with_alpha_animated.gif"
    "loadFromData/designer_rgb32.jpg"
    "loadFromData/designer_rgb32.png"
)

qt_internal_add_resource(tst_qpixmap "qpixmap"
    PREFIX
        "/"
    FILES
        ${qpixmap_resource_files}
)


## Scopes:
#####################################################################

qt_internal_extend_target(tst_qpixmap CONDITION TARGET Qt::Widgets
    LIBRARIES
        Qt::Widgets
        Qt::WidgetsPrivate
)

qt_internal_extend_target(tst_qpixmap CONDITION WIN32
    LIBRARIES
        gdi32
        user32
)
