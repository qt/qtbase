# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qset Test:
#####################################################################

qt_internal_add_test(tst_qset
    SOURCES
        tst_qset.cpp
)

qt_internal_undefine_global_definition(tst_qset QT_NO_JAVA_STYLE_ITERATORS)
