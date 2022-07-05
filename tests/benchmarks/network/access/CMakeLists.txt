# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from access.pro.

add_subdirectory(qfile_vs_qnetworkaccessmanager)
add_subdirectory(qnetworkreply)
add_subdirectory(qnetworkreply_from_cache)
add_subdirectory(qnetworkdiskcache)
if(QT_FEATURE_private_tests)
    add_subdirectory(qdecompresshelper)
endif()
