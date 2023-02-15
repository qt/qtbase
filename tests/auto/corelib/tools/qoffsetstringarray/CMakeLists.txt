# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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
