#!/usr/bin/env bash
# Copyright (C) 2018 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

set -ex

# export variables
export USER=qt-test-server
export PASS=password
export CONFIG=service/testdata
export TESTDATA=service/testdata

# add users
useradd -m -s /bin/bash $USER; echo "$USER:$PASS" | chpasswd

# install configurations and test data
su $USER -c "cp $CONFIG/system/passwords ~/"

./startup.sh "$@"
