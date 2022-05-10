#!/usr/bin/env bash
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

set -ex

# package ssl

# install ssl_certs and test data
su $USER -c "mkdir -p -m 700 ~/ssl-certs/private"
su $USER -c \
    "cp $CONFIG/ssl/${test_cert:-qt-test-server-cert.pem} ~/ssl-certs/qt-test-server-cert.pem"
su $USER -c "cp $CONFIG/ssl/private/qt-test-server-key.pem ~/ssl-certs/private/"
