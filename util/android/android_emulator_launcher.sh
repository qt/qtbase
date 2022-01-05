#!/bin/bash
#############################################################################
##
## Copyright (C) 2021 The Qt Company Ltd.
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

EMULATOR_MAX_RETRIES=5
EMULATOR_EXEC="$ANDROID_SDK_ROOT/emulator/emulator"
ADB_EXEC="$ANDROID_SDK_ROOT/platform-tools/adb"
if [[ -z "${ANDROID_EMULATOR}" ]]; then
    EMULATOR_NAME="@x86emulator"
else
    EMULATOR_NAME="$ANDROID_EMULATOR"
fi


function check_for_android_device
{
    $ADB_EXEC devices \
        | awk 'NR==2{print $2}' | grep -qE '^(online|device)$'
}

function check_if_fully_booted
{
    # The "getprop" command separates lines with \r\n so we trim them
    bootanim=`      $ADB_EXEC shell getprop init.svc.bootanim  | tr -d '\r\n'`
    boot_completed=`$ADB_EXEC shell getprop sys.boot_completed | tr -d '\r\n'`
    [ "$bootanim" = stopped ] && [ "$boot_completed" = 1 ]
}

function check_for_package_manager
{
(   set +e
    pm_output=`$ADB_EXEC shell pm get-install-location`
    pm_retval=$?
    [ $pm_retval = 0 ]  &&  [[ ! "$pm_output" =~ ^Error: ]]
)}

# NOTE: It happens that the device "finishes" booting and getprop shows
# bootanim=stopped and boot_completed=1. But sometimes not all packages have
# been installed (`pm list packages` shows only 16 packages installed), and
# after around half a minute the boot animation starts spinning
# (bootanim=running) again despite boot_completed=1 all the time. After some
# minutes the boot animation stops again and the list of packages contains 80
# packages, among which is also the "development" package.
function check_for_development_installed
{
    package_list=`$ADB_EXEC shell pm list packages`
    echo "$package_list" | grep -q package:com.android.development
}


for counter in `seq 1 ${EMULATOR_MAX_RETRIES}`; do
    $ADB_EXEC start-server

    if check_for_android_device
    then
        echo "Emulator is already running but it shouldn't be. Aborting\!"
        exit 3
    fi

    echo "Starting emulator, try ${counter}/${EMULATOR_MAX_RETRIES}"
    $EMULATOR_EXEC $EMULATOR_NAME  \
        -gpu swiftshader_indirect -no-audio -partition-size 4096  \
        -cores 4 -memory 3500 -no-snapshot-load -no-snapshot-save \
        </dev/null  >$HOME/emulator.log 2>&1  &
    emulator_pid=$!
    disown $emulator_pid

    $ADB_EXEC wait-for-device

    # Wait about one minute for the emulator to come up
    emulator_status=down
    for i in `seq 300`
    do
        sleep 1
        echo $i
        # Yes, you need to check continuously for all these heuristics,
        # see comment above
        if     check_for_android_device  \
            && check_if_fully_booted     \
            && check_for_package_manager \
            && check_for_development_installed
        then
            emulator_status=up
            break
        fi
    done

    # If emulator status is still offline after timeout period,
    # we can assume it's stuck, and we must restart it
    if [ $emulator_status = up ]
    then
        echo "Emulator started successfully"
        break
    else
        if [ $counter -lt $EMULATOR_MAX_RETRIES ]; then
            echo "Emulator failed to start, forcefully killing current instance and re-starting emulator"
            kill $emulator_pid || true
            sleep 5
        elif [ $counter -eq $EMULATOR_MAX_RETRIES ]; then
            echo "Emulator failed to start, reached maximum number of retries. Aborting\!"
            exit 2
        fi
    fi
done


exit 0
