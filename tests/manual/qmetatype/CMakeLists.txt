# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmetatype_expect_fail Test:
#####################################################################

qt_internal_add_test(tst_qmetatype_expect_fail
    GUI
    SOURCES
        declare_metatype_noninline.cpp declare_metatype_noninline.h
        tst_qmetatype.cpp
)
