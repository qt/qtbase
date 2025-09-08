# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qglobal Test:
#####################################################################

qt_internal_add_test(tst_qglobal
    SOURCES
        qglobal.c
        tst_qglobal.cpp
)

# check stuff (esp. qtypes.h) works in -ansi mode
qt_internal_add_test(tst_qglobal_strict
    SOURCES
        qglobal.c
        tst_qglobal.cpp
    DEFINES
        tst_QGlobal=tst_QGlobal_strict
    NO_PCH_SOURCES
        tst_qglobal.cpp # undef QT_NO_FOREACH
)
_qt_internal_apply_strict_cpp(tst_qglobal_strict)

## Scopes:
#####################################################################
