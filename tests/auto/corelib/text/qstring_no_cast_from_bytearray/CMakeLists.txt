# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qstring_no_cast_from_bytearray Test:
#####################################################################

qt_internal_add_test(tst_qstring_no_cast_from_bytearray
    SOURCES
        tst_qstring_no_cast_from_bytearray.cpp
    DEFINES
        QT_NO_CAST_FROM_BYTEARRAY
)
