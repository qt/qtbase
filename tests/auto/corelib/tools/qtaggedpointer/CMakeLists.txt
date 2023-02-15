# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtaggedpointer Test:
#####################################################################

qt_internal_add_test(tst_qtaggedpointer
    SOURCES
        tst_qtaggedpointer.cpp
    LIBRARIES
        Qt::CorePrivate
)
