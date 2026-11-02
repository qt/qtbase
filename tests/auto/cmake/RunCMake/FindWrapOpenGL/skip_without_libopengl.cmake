# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Without GLX, Qt falls back to linking libOpenGL from GLVND, which is not available everywhere.
find_library(test_libopengl NAMES OpenGL)
if(NOT test_libopengl)
    message(STATUS "Skipping test, libOpenGL was not found.")
    set(skip_test TRUE)
endif()
