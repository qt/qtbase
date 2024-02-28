# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

if(NOT TARGET Qt6::Concurrent)
    return()
endif()
if(TARGET Qt6::Widgets)
    qt_internal_add_example(imagescaling)
    qt_internal_add_example(primecounter)
    qt_internal_add_example(wordcount)
endif()
