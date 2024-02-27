#!/usr/bin/env bash
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

set -ex

# package squid

# install configurations and test data
cp $TESTDATA/squid{,-authenticating-ntlm}.conf /etc/squid/
sed -e 's,NAME=squid,NAME=squid-authenticating-ntlm,' \
    -e 's,CONFIG=/etc/squid/squid.conf,CONFIG=/etc/squid/squid-authenticating-ntlm.conf,' \
    -e 's,SQUID_ARGS="-YC -f $CONFIG",SQUID_ARGS="-D -YC -f $CONFIG",' \
    /etc/init.d/squid >/etc/init.d/squid-authenticating-ntlm
chmod +x /etc/init.d/squid-authenticating-ntlm

# enable service with installed configurations
service squid start
service squid-authenticating-ntlm start
