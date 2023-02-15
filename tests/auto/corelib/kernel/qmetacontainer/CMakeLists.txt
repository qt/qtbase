# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qmetacontainer Test:
#####################################################################

qt_internal_add_test(tst_qmetacontainer
    SOURCES
        tst_qmetacontainer.cpp
    LIBRARIES
        Qt::CorePrivate
)
