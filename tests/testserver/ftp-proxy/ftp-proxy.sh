#!/usr/bin/env bash
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

set -ex

# package ftp-proxy

# install configurations and test data
sed -i -e 's/# AllowMagicUser\tno/AllowMagicUser\tyes/' \
    -e 's/# ForkLimit\t\t40/ForkLimit\t\t2000/' \
    /etc/proxy-suite/ftp-proxy.conf

# enable service with installed configurations
ftp-proxy -d
