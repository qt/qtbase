# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## tst_qstringview Test:
#####################################################################

qt_internal_add_test(tst_qstringview
    SOURCES
        tst_qstringview.cpp
        arrays_of_unknown_bounds.cpp arrays_of_unknown_bounds.h
    LIBRARIES
        Qt::CorePrivate
)

if(QT_FEATURE_sanitize_undefined)
    qt_internal_extend_target(tst_qstringview
        DEFINES
            QT_SANITIZE_UNDEFINED # GCC (in)famously doesn't provide a predefined macro for this
    )
endif()

## Scopes:
#####################################################################
