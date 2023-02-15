# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qhash Test:
#####################################################################

qt_internal_add_test(tst_qhash
    SOURCES
        tst_qhash.cpp
)

qt_internal_undefine_global_definition(tst_qhash QT_NO_JAVA_STYLE_ITERATORS)
