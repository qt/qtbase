#!/bin/bash
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Runs an app on a simulator via xcrun simctl,
# or on a real device via xcrun devicectl.
#
# Usage: qt-apple-runner.sh <app_executable> [test_args...]
#
# <app_executable> is the binary inside the .app bundle, as passed by CTest
# when CROSSCOMPILING_EMULATOR is set. The bundle path is derived from it.
#
# Simulator: Set APPLE_SIMULATOR_UDID to target a specific booted simulator.
# If unset, the first booted simulator matching the app's platform is used.
#
# Device: Set APPLE_DEVICE_UDID to target a specific connected device.
# If unset, the first connected device is used.

die() { echo "qt-apple-runner.sh: $*" >&2; exit 254; }

app="$1"; shift
[ -n "$app" ] || die "⚠️ No app executable specified"

bundle=$(dirname "$app")
[ -d "$bundle" ] || die "⚠️ Bundle not found: $bundle"

bundle_id=$(/usr/libexec/PlistBuddy -c "Print:CFBundleIdentifier" \
    "$bundle/Info.plist" 2>/dev/null) \
    || die "⚠️ Failed to read CFBundleIdentifier from $bundle/Info.plist"

platform=$(/usr/libexec/PlistBuddy -c "Print:DTPlatformName" \
    "$bundle/Info.plist" 2>/dev/null)

case "$platform" in
    *simulator) target_type=simulator ;;
    *)          target_type=device ;;
esac

case "$platform" in
    iphone*)    runtime="iOS" ;;
    appletv*)   runtime="tvOS" ;;
    watch*)     runtime="watchOS" ;;
    xr*)        runtime="xrOS" ;;
    *)          runtime="" ;;
esac

forward_env() {
    local prefix="$1"
    while IFS='=' read -r key rest; do
        case "$key" in
            LC_*|LANG) ;;
            *) [ -n "$key" ] && export "${prefix}${key}=${rest}" ;;
        esac
    done < <(env)
}

# Let the platform integration know it should set a custom cwd
# and write the exit code to a file.
export QT_RUNNING_VIA_TEST_RUNNER=1

if [ "$target_type" = "simulator" ]; then
    if [ -n "$APPLE_SIMULATOR_UDID" ]; then
        udid="$APPLE_SIMULATOR_UDID"
    else
        udid=$(xcrun simctl list devices booted --json 2>/dev/null \
            | jq -r --arg f "$runtime" \
                '.devices | to_entries[]
                 | select($f == "" or (.key | contains($f)))
                 | .value[] | select(.state == "Booted") | .udid' \
            | head -1)
        [ -n "$udid" ] || die "⚠️ No booted $runtime simulator found; boot one or set APPLE_SIMULATOR_UDID"
    fi

    xcrun simctl install "$udid" "$bundle" \
        || die "⚠️ Failed to install $bundle on simulator $udid"

    forward_env "SIMCTL_CHILD_"

    xcrun simctl launch --console-pty "$udid" "$bundle_id" "$@" \
        || die "⚠️ Failed to launch $bundle on simulator $udid"

    tmp_dir="$(xcrun simctl get_app_container "$udid" "$bundle_id" data)/tmp" \
        || die "⚠️ Failed to locate data container for $bundle_id on simulator $udid"
else
    if [ -n "$APPLE_DEVICE_UDID" ]; then
        udid="$APPLE_DEVICE_UDID"
    else
        udid=$(xcrun devicectl list devices --quiet --json-output /dev/stdout 2>/dev/null \
            | jq -r --arg r "$runtime" \
                '.result.devices[]
                 | select(.connectionProperties.tunnelState == "connected")
                 | select($r == "" or .hardwareProperties.platform == $r)
                 | .hardwareProperties.udid' \
            | head -1)
        [ -n "$udid" ] || die "⚠️ No connected $runtime device found; connect one or set APPLE_DEVICE_UDID"
    fi

    xcrun devicectl device install app --device "$udid" "$bundle" \
        || die "⚠️ Failed to install $bundle on device $udid"

    forward_env "DEVICECTL_CHILD_"

    # Note: devicectl doesn't reliably propagate the app's exit code (e.g. it reports
    # 0 even when dyld fails to load a library), so we don't trust the launch exit code
    # and rely on the one Qt writes to a file in the data container.
    xcrun devicectl device process launch --console \
        --device "$udid" -- "$bundle_id" "$@" || true

    # Stage a local copy of the data container's tmp/ so the post-processing
    # below can treat device and simulator the same way.
    tmp_dir=$(mktemp -d) || die "⚠️ Failed to resolve tmp dir"
    trap 'rm -rf "$tmp_dir"' EXIT
    xcrun devicectl device copy from \
        --quiet \
        --device "$udid" \
        --domain-type appDataContainer \
        --domain-identifier "$bundle_id" \
        --source "tmp" \
        --destination "$tmp_dir" 2>/dev/null || true
fi

exit_code=$(cat "$tmp_dir/qt_exit_code.txt" 2>/dev/null)

if [ -d "$tmp_dir/testrunner" ]; then
    cp -r "$tmp_dir/testrunner/." .
fi

exit "${exit_code:-253}"
