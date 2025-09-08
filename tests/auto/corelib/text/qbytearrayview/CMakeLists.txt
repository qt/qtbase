# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qbytearrayview Test:
#####################################################################

qt_internal_add_test(tst_qbytearrayview
    SOURCES
        tst_qbytearrayview.cpp
)

if(QT_FEATURE_sanitize_undefined)
    qt_internal_extend_target(tst_qbytearrayview
        DEFINES
            QT_SANITIZE_UNDEFINED # GCC (in)famously doesn't provide a predefined macro for this
    )
endif()
