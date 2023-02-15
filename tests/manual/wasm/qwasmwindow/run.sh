#! /bin/bash

# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

set -m

function removeServer()
{
    [ -z "$cleanupPid" ] || kill $cleanupPid
}

trap removeServer EXIT

script_dir=`dirname ${BASH_SOURCE[0]}`
cd "$script_dir"
python3 qtwasmserver.py -p 8001 > /dev/null 2>&1 &
cleanupPid=$!

python3 qwasmwindow.py $@

echo 'Press any key to continue...' >&2
read -n 1
