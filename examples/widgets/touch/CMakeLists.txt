# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT TARGET Qt6::Widgets)
    return()
endif()
qt_internal_add_example(pinchzoom)
qt_internal_add_example(fingerpaint)
qt_internal_add_example(knobs)
qt_internal_add_example(dials)
