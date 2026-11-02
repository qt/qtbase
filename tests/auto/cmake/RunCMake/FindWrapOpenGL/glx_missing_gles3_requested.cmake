# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

include(skip_without_libopengl.cmake)
if(skip_test)
    return()
endif()

# Simulate a missing gl / glx library.
# Qt should fall back to libOpenGL (OpenGL::OpenGL).
set(OPENGL_glx_LIBRARY OFF CACHE FILEPATH "")
set(OPENGL_gl_LIBRARY OFF CACHE FILEPATH "")

find_package(OpenGL COMPONENTS GLES3)
include(find_qt_opengl.cmake)
