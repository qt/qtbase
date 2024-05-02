# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
qt_internal_add_manual_test(echo_client_secondarythread
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
        Qt::Network
)
