# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Simulate a missing GLES3 library.
# Because -DOPENGL_USE_GLES3=ON is passed as a cache var, which Qt respects, configuration
# should fail.
set(OPENGL_gles3_LIBRARY OFF CACHE FILEPATH "")

find_package(Qt6 REQUIRED COMPONENTS OpenGL)
