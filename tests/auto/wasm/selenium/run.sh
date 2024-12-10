#! /bin/bash

# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

set -m

function removeServer()
{
    [ -z "$cleanupPid" ] || kill $cleanupPid
}
trap removeServer EXIT

function compare_python_versions() {
    python_version=$1
    required_version=$2
    if [ "$(printf "%s\n" "$required_version" "$python_version" | sort -V | head -n 1)" != "$required_version" ]; then
        return 0  # python version is too old
    else
        return 1  # python version is legit
    fi
}

python_command="python3"
# At least python 3.7 is required for Selenium 4
if command -v python3 &> /dev/null; then
    python_version=$(python3 --version 2>&1 | awk '{print $2}')

    if compare_python_versions "$python_version" "3.7"; then # if Python is older then 3.7, look for newer versions
        newer_python=""
        for version in 3.7 3.8 3.9 3.10 3.11; do
            potential_python=$(command -v "python$version")
            if [ -n "$potential_python" ]; then
                newer_python=$potential_python
                newer_version=$($newer_python --version 2>&1 | awk '{print $2}')
                break
            fi
        done

        if [ -n "$newer_python" ]; then # if newer version is found, use it instead
            newer_version=$($newer_python --version 2>&1 | awk '{print $2}')
            python_command=$newer_python
        else
            echo "Error: At least Python3.7 is required, currently installed version: Python$python_version"
            exit 1
        fi
    fi
else
    echo "Error: Python3 not installed"
    exit 1
fi

script_dir=`dirname ${BASH_SOURCE[0]}`
cd "$script_dir"
$python_command qtwasmserver.py --cross-origin-isolation -p 8001 > /dev/null 2>&1 &
cleanupPid=$!

$python_command qwasmwindow.py $@
