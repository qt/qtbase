# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qbytearray Test:
#####################################################################

qt_internal_add_test(tst_qbytearray
    SOURCES
        tst_qbytearray.cpp
    LIBRARIES
        Qt::CorePrivate
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qbytearray CONDITION APPLE
    SOURCES
        tst_qbytearray_mac.mm
    LIBRARIES
        ${FWFoundation}
)
