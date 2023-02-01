#!/bin/bash
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0
# This util launches the Android emulator and ensures it doesn't stuck/freeze
# by detecting that and restarting it

set -e


EMULATOR_MAX_RETRIES=5
EMULATOR_EXEC="$ANDROID_SDK_ROOT/emulator/emulator"
ADB_EXEC="$ANDROID_SDK_ROOT/platform-tools/adb"
if [ -z "${ANDROID_EMULATOR}" ]
then
    EMULATOR_NAME="@emulator_x86_api_23"
else
    EMULATOR_NAME="$ANDROID_EMULATOR"
fi


function check_for_android_device
{
    $ADB_EXEC devices \
        | awk 'NR==2{print $2}' | grep -qE '^(online|device)$'
}

# WARNING: On the very first boot of the emulator it happens that the device
# "finishes" booting and getprop shows bootanim=stopped and
# boot_completed=1. But sometimes not all packages have been installed (`pm
# list packages` shows only 16 packages installed), and after around half a
# minute the boot animation starts spinning (bootanim=running) again despite
# boot_completed=1 all the time. After some minutes the boot animation stops
# again and the list of packages contains 80 packages. Only then the device is
# fully booted, and only then is dev.bootcomplete=1.
#
# To reproduce the emulator booting as the first time, you have to delete the
# cached images found inside $HOME/.android especially the
# "userdata-qemu.img.qcow2" file.
function check_if_fully_booted
{
    # The "getprop" command separates lines with \r\n so we trim them
    bootanim=`      $ADB_EXEC shell getprop init.svc.bootanim  | tr -d '\r\n'`
    boot_completed=`$ADB_EXEC shell getprop sys.boot_completed | tr -d '\r\n'`
    bootcomplete=`  $ADB_EXEC shell getprop dev.bootcomplete   | tr -d '\r\n'`
    echo "bootanim=$bootanim boot_completed=$boot_completed bootcomplete=$bootcomplete"
    [ "$bootanim" = stopped ] && [ "$boot_completed" = 1 ] && [ "$bootcomplete" = 1 ]
}



for counter in `seq ${EMULATOR_MAX_RETRIES}`
do
    $ADB_EXEC start-server

    if check_for_android_device
    then
        echo "Emulator is already running but it shouldn't be. Aborting\!"
        exit 3
    fi

    echo "Starting emulator, try ${counter}/${EMULATOR_MAX_RETRIES}"
    $EMULATOR_EXEC $EMULATOR_NAME  \
        -gpu swiftshader_indirect -no-audio -partition-size 4096  \
        -cores 4 -memory 16000 -no-snapshot-load -no-snapshot-save \
        </dev/null  >$HOME/emulator.log 2>&1  &
    emulator_pid=$!
    disown $emulator_pid

    echo "Waiting for emulated device to appear..."
    $ADB_EXEC wait-for-device

    echo "Waiting a few minutes for the emulator to fully boot..."
    emulator_status=down
    for i in `seq 300`
    do
        sleep 1

        if  check_for_android_device  &&  check_if_fully_booted
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
        if [ $counter -lt $EMULATOR_MAX_RETRIES ]
        then
            echo "Emulator failed to start, forcefully killing current instance and re-starting emulator"
            kill $emulator_pid || true
            sleep 5
        elif [ $counter -eq $EMULATOR_MAX_RETRIES ]
        then
            echo "Emulator failed to start, reached maximum number of retries. Aborting\!"
            exit 2
        fi
    fi
done


exit 0
