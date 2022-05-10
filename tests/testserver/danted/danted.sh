#!/usr/bin/env bash
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

set -ex

# package dante-server

# add users
useradd -d /dev/null -s /bin/false qsockstest; echo "qsockstest:$PASS" | chpasswd

# install configurations and test data
cp $TESTDATA/danted{,-authenticating}.conf /etc/

# Use the input environment variables to overwrite the default value of internal interfaces.
if [ "$danted_internal" -a "$danted_internal" != eth0 ]
then sed -i "s,internal: eth0 port = 1080,internal: $danted_internal port = 1080," /etc/danted.conf
fi

if [ "$danted_auth_internal" -a "$danted_auth_internal" != eth0 ]
then sed -i "s,internal: eth0 port = 1081,internal: $danted_auth_internal port = 1081," \
     /etc/danted-authenticating.conf
fi

# Use the input environment variables to overwrite the default value of external interfaces.
if [ "$danted_external" -a "$danted_external" != eth0 ]
then sed -i "s,external: eth0,external: $danted_external," /etc/danted.conf
fi

if [ "$danted_auth_external" -a "$danted_auth_external" != eth0 ]
then sed -i "s,external: eth0,external: $danted_auth_external," /etc/danted-authenticating.conf
fi

# enable service with installed configurations
service danted start
service danted-authenticating start
