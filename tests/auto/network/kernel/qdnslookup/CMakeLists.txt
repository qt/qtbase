# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdnslookup Test:
#####################################################################

qt_internal_add_test(tst_qdnslookup
    SOURCES
        tst_qdnslookup.cpp
    LIBRARIES
        Qt::Network
        Qt::TestPrivate
)

qt_internal_extend_target(tst_qdnslookup CONDITION WIN32
    LIBRARIES
        iphlpapi
)
