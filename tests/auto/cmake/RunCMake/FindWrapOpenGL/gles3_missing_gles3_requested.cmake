# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Simulate a missing GLES3 library.
# Because Qt doesn't need GLES3, it should still find desktop OpenGL and configuration succeeds.
set(OPENGL_gles3_LIBRARY OFF CACHE FILEPATH "")

find_package(OpenGL COMPONENTS GLES3)
include(find_qt_opengl.cmake)
