# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(QT_FEATURE_private_tests)
    add_subdirectory(qhttpsocketengine)
    add_subdirectory(qtcpsocket)
    add_subdirectory(qsocks5socketengine)
    add_subdirectory(platformsocketengine)
endif()
if(QT_FEATURE_udpsocket)
    add_subdirectory(qudpsocket)
endif()
add_subdirectory(qabstractsocket)
if(QT_FEATURE_localserver AND NOT ANDROID)
    # QTBUG-87387
    add_subdirectory(qlocalsocket)
endif()

if(QT_FEATURE_networkinterface AND NOT ANDROID)
    # QTBUG-87388
    add_subdirectory(qtcpserver)
endif()

if(QT_FEATURE_sctp)
    add_subdirectory(qsctpsocket)
endif()
