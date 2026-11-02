# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Simulate a missing GLES3 library and a leaked OPENGL_USE_GLES3.
# Set OPENGL_USE_GLES3 directly because newer CMake versions may not leak it.
# With -DQT_NO_OVERRIDE_OPENGL_USE_VARIABLES=ON set, configuration should fail.
set(OPENGL_gles3_LIBRARY OFF CACHE FILEPATH "")
set(OPENGL_USE_GLES3 1)

find_package(Qt6 REQUIRED COMPONENTS OpenGL)
