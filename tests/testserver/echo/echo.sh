#!/usr/bin/env bash
# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Disabled by default, enable it.
sed -i 's/disable\t\t= yes/disable = no/' /etc/xinetd.d/echo
sed -i 's/disable\t\t= yes/disable = no/' /etc/xinetd.d/daytime

service xinetd restart
