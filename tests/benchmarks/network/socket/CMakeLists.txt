# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_FEATURE_localserver)
    add_subdirectory(qlocalsocket)
endif()

if(QT_FEATURE_udpsocket)
    add_subdirectory(qudpsocket)
endif()

add_subdirectory(qtcpserver)
