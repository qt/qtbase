# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

add_library(network_cppsnippets OBJECT tcpwait.cpp)

target_link_libraries(network_cppsnippets PRIVATE
    Qt::Network
)
