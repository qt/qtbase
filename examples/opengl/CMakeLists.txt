# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_example(openglwindow)
qt_internal_add_example(hellogles3)
if(TARGET Qt6::Widgets)
    qt_internal_add_example(contextinfo)
    qt_internal_add_example(2dpainting)
    qt_internal_add_example(hellogl2)
    qt_internal_add_example(cube)
    qt_internal_add_example(textures)
    qt_internal_add_example(stereoqopenglwidget)
endif()
