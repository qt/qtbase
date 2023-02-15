# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qwineventnotifier Test:
#####################################################################

qt_internal_add_test(tst_qwineventnotifier
    SOURCES
        tst_qwineventnotifier.cpp
    LIBRARIES
        Qt::CorePrivate
)
