# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qflags Test:
#####################################################################

qt_internal_add_test(tst_qflags
    SOURCES
        tst_qflags.cpp
)

qt_internal_add_test(tst_qflags_non_typesafe
    SOURCES
        tst_qflags.cpp
    DEFINES
        QFLAGS_TEST_NO_TYPESAFE_FLAGS
)
