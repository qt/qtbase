# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qhashseed Test:
#####################################################################

qt_internal_add_test(tst_qhashseed
    SOURCES
        tst_qhashseed.cpp
)

qt_internal_add_executable(tst_qhashseed_helper
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/"
    SOURCES
        tst_qhashseed_helper.cpp
)
