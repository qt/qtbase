# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_manual_qsslsocket Test:
#####################################################################

qt_internal_add_test(tst_manual_qsslsocket
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Network
)
