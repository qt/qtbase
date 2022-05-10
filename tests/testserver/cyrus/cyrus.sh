#!/usr/bin/env bash
# Copyright (C) 2019 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

set -ex

echo "tls_cert_file: /home/qt-test-server/ssl-certs/qt-test-server-cert.pem" >> /etc/imapd.conf
echo "tls_key_file: /home/qt-test-server/ssl-certs/private/qt-test-server-key.pem" >> /etc/imapd.conf
chmod +3 /home/qt-test-server/ssl-certs/private/
mkdir -m 007 -p /run/cyrus/proc
sed -i 's/#imaps\t\tcmd="imapd/imaps\t\tcmd="imapd/' /etc/cyrus.conf

service cyrus-imapd restart
