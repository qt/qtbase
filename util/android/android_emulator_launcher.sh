#!/bin/bash
# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# This util launches the Android emulator and ensures it doesn't stuck/freeze
# by detecting that and restarting it

set -e

EMULATOR_MAX_RETRIES=3
ADB_MAX_TIMEOUT=180
EMULATOR_EXEC="$ANDROID_SDK_ROOT/emulator/emulator"
ADB_EXEC="$ANDROID_SDK_ROOT/platform-tools/adb"
LOGCAT_PATH="$COIN_CTEST_RESULTSDIR/emulator_logcat_%iter.txt"
EMULATOR_RUN_LOG_PATH="$COIN_CTEST_RESULTSDIR/emulator_run_log_%iter.txt"

if [ -z "${ANDROID_EMULATOR}" ]; then
    echo "No AVD name provided via ANDROID_EMULATOR env variable. Aborting!"
    exit 1
fi
ANDROID_EMULATOR="${ANDROID_EMULATOR#@}"

function check_for_android_device
{
    $ADB_EXEC devices | awk 'NR==2{print $2}' | grep -qE '^(online|device)$'
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
# cached images found inside $HOME/.android/avd/{avd_name}.avd/ especially the
# "userdata-qemu.img.qcow2" file.
function check_if_fully_booted
{
    # The "getprop" command separates lines with \r\n so we trim them
    bootanim=$(      timeout 1 "$ADB_EXEC" shell getprop init.svc.bootanim  | tr -d '\r\n')
    boot_completed=$(timeout 1 "$ADB_EXEC" shell getprop sys.boot_completed | tr -d '\r\n')
    bootcomplete=$(  timeout 1 "$ADB_EXEC" shell getprop dev.bootcomplete   | tr -d '\r\n')
    echo "bootanim=$bootanim boot_completed=$boot_completed bootcomplete=$bootcomplete"
    [ "$bootanim" = stopped ] && [ "$boot_completed" = 1 ] && [ "$bootcomplete" = 1 ]
}

for counter in $(seq ${EMULATOR_MAX_RETRIES})
do
    $ADB_EXEC kill-server
    $ADB_EXEC start-server
    sleep 1
    if check_for_android_device
    then
        echo "Emulator is already running but it shouldn't be. Terminating it now..."
        pkill '^qemu-system-' || true
        sleep 5
    fi

    LOGCAT_PATH=${LOGCAT_PATH//%iter/${counter}}
    EMULATOR_RUN_LOG_PATH=${EMULATOR_RUN_LOG_PATH//%iter/${counter}}

    echo "Starting emulator ${ANDROID_EMULATOR}, try ${counter}/${EMULATOR_MAX_RETRIES}" \
        | tee "${EMULATOR_RUN_LOG_PATH}"
    $EMULATOR_EXEC -avd "$ANDROID_EMULATOR" \
        -gpu swiftshader_indirect -no-audio -no-window -no-boot-anim \
        -cores 4 -memory 16000 -partition-size 4096 \
        -detect-image-hang -restart-when-stalled -no-snapshot-save \
        -no-nested-warnings -logcat '*:v' -logcat-output "${LOGCAT_PATH}" \
        </dev/null  >"${EMULATOR_RUN_LOG_PATH}" 2>&1 &
    emulator_pid=$!
    disown $emulator_pid

    echo "Waiting ${ADB_MAX_TIMEOUT} seconds for emulated device to appear..."
    timeout ${ADB_MAX_TIMEOUT} "$ADB_EXEC" wait-for-device

    # Due to some bug in Coin/Go, we can't have the emulator command stream
    # the output to the console while in the background, as Coin will continue
    # waiting for it. So, rely on re-directing all output to a log file and
    # then printing it out after the emulator is started.
    echo "######## Printing out the emulator command logs ########"
    cat "${EMULATOR_RUN_LOG_PATH}"
    echo "########################################################"

    echo "Waiting a few minutes for the emulator to fully boot..."
    emulator_status=down

    time_start=${SECONDS}
    duration=0

    while [ $duration -lt ${ADB_MAX_TIMEOUT} ]
    do
        sleep 1

        if check_for_android_device && check_if_fully_booted
        then
            emulator_status=up
            break
        fi
        duration=$(( SECONDS - time_start ))
    done

    # If emulator status is still offline after timeout period,
    # we can assume it's stuck, and we must restart it
    if [ $emulator_status = up ]
    then
        echo "Emulator started successfully"
        break
    else
        if [ "$counter" -lt "$EMULATOR_MAX_RETRIES" ]
        then
            echo "Emulator failed to start," \
                 "forcefully killing current instance and re-starting emulator"
            kill $emulator_pid || true
            sleep 5
        elif [ "$counter" -eq "$EMULATOR_MAX_RETRIES" ]
        then
            echo "Emulator failed to start, reached maximum number of retries. Aborting\!"
            exit 2
        fi
    fi
done

exit 0
