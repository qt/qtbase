# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(keypadnavigation)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/suite_keypadnavigation"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/suite_keypadnavigation")

