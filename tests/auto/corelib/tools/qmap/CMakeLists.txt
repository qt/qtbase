# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmap Test:
#####################################################################

qt_internal_add_test(tst_qmap
    SOURCES
        tst_qmap.cpp
)

qt_internal_undefine_global_definition(tst_qmap QT_NO_JAVA_STYLE_ITERATORS)
