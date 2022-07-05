# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_internal_add_example(semaphores)
qt_internal_add_example(waitconditions)
if(TARGET Qt::Widgets)
    qt_internal_add_example(mandelbrot)
    qt_internal_add_example(queuedcustomtype)
endif()
