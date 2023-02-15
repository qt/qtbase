# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## acquirerelease_helper Binary:
#####################################################################

qt_internal_add_test_helper(acquirerelease_helper
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Test
)
