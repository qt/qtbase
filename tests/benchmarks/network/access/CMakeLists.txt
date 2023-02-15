# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

add_subdirectory(qfile_vs_qnetworkaccessmanager)
add_subdirectory(qnetworkreply)
add_subdirectory(qnetworkreply_from_cache)
add_subdirectory(qnetworkdiskcache)
if(QT_FEATURE_private_tests)
    add_subdirectory(qdecompresshelper)
endif()
