#!/usr/bin/env bash

#############################################################################
##
## Copyright (C) 2018 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the test suite of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 or (at your option) any later version
## approved by the KDE Free Qt Foundation. The licenses are as published by
## the Free Software Foundation and appearing in the file LICENSE.GPL3
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

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
