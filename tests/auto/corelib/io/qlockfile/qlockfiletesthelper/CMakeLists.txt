# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qlockfile_test_helper Binary:
#####################################################################

qt_internal_add_test_helper(qlockfile_test_helper
    OVERRIDE_OUTPUT_DIRECTORY
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        qlockfile_test_helper.cpp
)
