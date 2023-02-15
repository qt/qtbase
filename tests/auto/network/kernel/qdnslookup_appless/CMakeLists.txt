# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qdnslookup_appless Test:
#####################################################################

qt_internal_add_test(tst_qdnslookup_appless
    SOURCES
        tst_qdnslookup_appless.cpp
    LIBRARIES
        Qt::Network
)
