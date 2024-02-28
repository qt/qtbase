# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_internal_add_example(hellovulkantriangle)
if(TARGET Qt6::Widgets)
    qt_internal_add_example(hellovulkanwidget)
endif()
if(TARGET Qt6::Concurrent AND TARGET Qt6::Widgets)
    qt_internal_add_example(hellovulkancubes)
endif()
