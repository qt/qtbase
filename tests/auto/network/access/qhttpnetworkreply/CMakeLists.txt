# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qhttpnetworkreply.pro.

if(NOT QT_FEATURE_private_tests)
    return()
endif()

#####################################################################
## tst_qhttpnetworkreply Test:
#####################################################################

qt_internal_add_test(tst_qhttpnetworkreply
    SOURCES
        tst_qhttpnetworkreply.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::NetworkPrivate
)

#### Keys ignored in scope 1:.:.:qhttpnetworkreply.pro:<TRUE>:
# _REQUIREMENTS = "qtConfig(private_tests)"
