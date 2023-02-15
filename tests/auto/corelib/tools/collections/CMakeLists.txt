# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_collections Test:
#####################################################################

qt_internal_add_test(tst_collections
    SOURCES
        tst_collections.cpp
)

qt_internal_undefine_global_definition(tst_collections QT_NO_JAVA_STYLE_ITERATORS)
