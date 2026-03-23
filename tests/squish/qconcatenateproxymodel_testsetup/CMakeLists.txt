# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qconcatenatetablesproxymodel)

if(NOT CMAKE_CURRENT_BINARY_DIR STREQUAL CMAKE_CURRENT_SOURCE_DIR)
    file(COPY "${CMAKE_CURRENT_SOURCE_DIR}/suite_qconcatenateproxymodel"
         DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/suite_qconcatenateproxymodel")
endif()
