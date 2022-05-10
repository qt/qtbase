#!/usr/bin/env bash
# Copyright (C) 2019 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

set -ex

iptables -A INPUT -p tcp --destination-port 1357 -j DROP
