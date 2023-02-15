# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qtemporarydir Test:
#####################################################################

qt_internal_add_test(tst_qtemporarydir
    SOURCES
        tst_qtemporarydir.cpp
    LIBRARIES
        Qt::TestPrivate
)

qt_internal_extend_target(tst_qtemporarydir CONDITION WIN32
    LIBRARIES
        shlwapi
)
