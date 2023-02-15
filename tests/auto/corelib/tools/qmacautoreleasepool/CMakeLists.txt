# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmacautoreleasepool Test:
#####################################################################

qt_internal_add_test(tst_qmacautoreleasepool
    SOURCES
        tst_qmacautoreleasepool.mm
    LIBRARIES
        Qt::CorePrivate
        ${FWFoundation}
)
