# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qlayout)

file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/suite_qlayout"
     DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/suite_qlayout")
