# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_internal_add_example(hellovulkanwindow)
qt_internal_add_example(hellovulkantriangle)
qt_internal_add_example(hellovulkantexture)
if(TARGET Qt6::Widgets)
    qt_internal_add_example(hellovulkanwidget)
endif()
if(TARGET Qt6::Concurrent AND TARGET Qt6::Widgets)
    qt_internal_add_example(hellovulkancubes)
endif()
