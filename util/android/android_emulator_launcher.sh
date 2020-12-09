#!/bin/sh
#############################################################################
##
## Copyright (C) 2020 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the plugins of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
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
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################
# This util launches the Android emulator and ensures it doesn't stuck/freeze
# by detecting that and restarting it

set -ex

EMULATOR_TIMEOUT=30
EMULATOR_MAX_RETRIES=5
EMULATOR_EXEC="$ANDROID_SDK_HOME/tools/emulator"
ADB_EXEC="$ANDROID_SDK_HOME/platform-tools/adb"
EMULATOR_NAME="@x86emulator"
RESULT=0

for counter in `seq 1 ${EMULATOR_MAX_RETRIES}`; do
    $ADB_EXEC start-server
    echo "Starting emulator, try ${counter}/${EMULATOR_MAX_RETRIES}"
    $EMULATOR_EXEC $EMULATOR_NAME -no-audio -partition-size 4096 -cores 4 -memory 3500 -no-snapshot-load -no-snapshot-save &>/dev/null &
    emulator_pid=$!

    # Give emulator time to start
    sleep $EMULATOR_TIMEOUT

    emulator_status=`$ADB_EXEC devices | tail -n -2 | awk '{print $2}'`

    # If emulator status is still offline after timeout period,
    # we can assume it's stuck, and we must restart it
    if [[ $emulator_status == 'online'  || $emulator_status == 'device' ]]; then
        echo "Emulator started successfully"
        break
    else
        if [ $counter -lt $EMULATOR_MAX_RETRIES ]; then
            echo "Emulator failed to start, forcefully killing current instance"
            kill $emulator_pid || true
            sleep 5
        elif [ $counter -eq $EMULATOR_MAX_RETRIES ]; then
            echo "Emulator failed to start, reached maximum number of retries"
            RESULT=-1
            break
        fi
    fi
done

exit $RESULT
