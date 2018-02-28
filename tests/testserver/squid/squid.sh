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
