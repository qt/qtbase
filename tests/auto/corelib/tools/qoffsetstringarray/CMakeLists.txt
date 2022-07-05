# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qoffsetstringarray.pro.

#####################################################################
## tst_qoffsetstringarray Test:
#####################################################################

qt_internal_add_test(tst_qoffsetstringarray
    SOURCES
        tst_qoffsetstringarray.cpp
    LIBRARIES
        Qt::CorePrivate
)

if (CLANG)
    target_compile_options(tst_qoffsetstringarray
        PUBLIC -fbracket-depth=512)
elseif (GCC)
    # fconstexpr-depth= defaults to 512
endif()
