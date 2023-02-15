# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qstringmatcher Test:
#####################################################################

qt_internal_add_test(tst_qstringmatcher
    SOURCES
        tst_qstringmatcher.cpp
    DEFINES
        QT_NO_CAST_TO_ASCII
)
