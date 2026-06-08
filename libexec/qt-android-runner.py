#!/usr/bin/env python3
# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import atexit
import os
import shlex
import subprocess
import sys
import base64
import time
import signal
import argparse
import tempfile
import xml.etree.ElementTree as ET

def status(msg):
    print(f"\n-- {msg}")

def error(msg):
    print(f"Error: {msg}", file=sys.stderr)

def die(msg):
    error(msg)
    sys.exit(1)

# Define and parse arguments
parser = argparse.ArgumentParser(description="Qt for Android app runner.",
                                 epilog='''
This is a helper script to run Qt for Android apps directly from the terminal.
It supports starting apps with parameters and forwards environment variables to
the device. It prints live logcat messages as the app is running. The script exits
once the app has exited on the device and terminates the app on the device if the
script is terminated.

If an APK path is provided, it will first be installed to the device only if the
--install parameter is passed.

Use --serial parameter or adb's ANDROID_SERIAL environment variable to specify an
Android target serial number (obtained from "adb devices" command) on which to run
the app or test.
''', formatter_class=argparse.RawTextHelpFormatter)

parser.add_argument('-a', '--adb', metavar='path', type=str, help='Path to adb executable.')
parser.add_argument('-b', '--build-path', metavar='path', type=str,
                    help='Path to the Android build directory.')
parser.add_argument('-i', '--install', action='store_true', help='Install the APK.')
parser.add_argument('-d', '--detached', action='store_true',
                    help='Start the app detached without waiting for the logcat')
parser.add_argument('-s', '--serial', type=str, metavar='serial',
                    help='Android device serial (override $ANDROID_SERIAL).')
parser.add_argument('-p', '--apk', type=str, metavar='path', help='Path to the APK file.')

args, remaining_args = parser.parse_known_args()

# Validate required arguments
if not args.build_path:
    die("App build path is not provided")

adb = args.adb
if not adb:
    adb = 'adb'
    null_dev = subprocess.DEVNULL
    if subprocess.call(['command', '-v', adb], stdout=null_dev, stderr=null_dev) != 0:
        die("adb tool path is not provided and is not found in PATH")

try:
    devices = []
    output = subprocess.check_output(f"{adb} devices", shell=True).decode().strip()
    for line in output.splitlines():
        if '\tdevice' in line:
            serial = line.split('\t')[0]
            devices.append(serial)
    if not devices:
        die("No devices are connected.")

    if args.serial and args.serial not in devices:
        die("No connected devices with the specified serial number.")
except Exception as e:
    die(f"Failed to check for running devices, received error: {e}")

# Keep both forms: adb_argv for subprocess.run(list) calls below, and a
# shlex-joined string so the existing f-string shell=True usages still work.
adb_argv = [adb]
if args.serial:
    adb_argv += ["-s", args.serial]
adb = shlex.join(adb_argv)

if args.build_path is None:
    die("App build path is not provided")

if args.apk and args.install:
    status(f"Installing the app APK {args.apk}")
    try:
        subprocess.run(f"{adb} install \"{args.apk}\"", check=True, shell=True)
    except Exception as e:
        error(f"Failed to install the APK, received error: {e}")


def find_launcher_activity(root):
    ns_android = 'http://schemas.android.com/apk/res/android'
    android_name_attr = f'{{{ns_android}}}name'

    for activity in root.findall('.//activity'):
        for intent_filter in activity.findall('intent-filter'):
            actions = {action.get(android_name_attr) for action in intent_filter.findall('action')}
            categories = {cat.get(android_name_attr) for cat in intent_filter.findall('category')}
            main_action = 'android.intent.action.MAIN'
            launcher_category = 'android.intent.category.LAUNCHER'
            if main_action in actions and launcher_category in categories:
                return activity.get(android_name_attr)
    return None


def get_manifest_activity(manifest_file):
    try:
        if not os.path.isfile(manifest_file):
            return None

        tree = ET.parse(manifest_file)
        root = tree.getroot()
        return find_launcher_activity(root)
    except Exception as e:
        error(f"Failed to parse AndroidManifest.xml, received error: {e}")
        return None

gradle_init_script_path = None

def gradle_init_script():
    """Return path to a reusable Gradle init-script for property queries. Created once."""
    global gradle_init_script_path
    if gradle_init_script_path and os.path.isfile(gradle_init_script_path):
        return gradle_init_script_path

    fd, path = tempfile.mkstemp(suffix=".gradle")
    with os.fdopen(fd, 'w') as f:
        f.write(
            'gradle.projectsEvaluated {\n'
            '    def prop = gradle.rootProject.findProperty("property")\n'
            '    def target = gradle.rootProject.findProject(":app") ?: gradle.rootProject\n'
            '    for (part in prop.tokenize(".")) {\n'
            '        target = target."${part}"\n'
            '    }\n'
            '    println target\n'
            '}\n'
            'rootProject { tasks.register("printProjectProperty") }\n'
        )
    atexit.register(lambda: os.path.exists(path) and os.unlink(path))
    gradle_init_script_path = path
    return path

# Query a Gradle project's property using dot-separated path notation e.g. "android.namespace".
def get_gradle_project_property(build_path, property_path):
    gradlew_name = "gradlew.bat" if sys.platform == "win32" else "gradlew"
    gradlew = os.path.join(build_path, gradlew_name)
    if not os.path.isfile(gradlew):
        # Look at the parent dir in multi-module Gradle layout
        gradlew = os.path.join(build_path, os.pardir, gradlew_name)
    if not os.path.isfile(gradlew):
        return None

    script_path = gradle_init_script()
    if not script_path:
        return None

    try:
        gradle_root = os.path.dirname(gradlew)
        property_arg = f"-Pproperty={property_path}"
        result = subprocess.run(
            [gradlew, "-q", "--init-script", script_path, property_arg, "printProjectProperty"],
            cwd=gradle_root, capture_output=True, text=True, timeout=30
        )
        value = result.stdout.strip()
        return value if value else None
    except Exception as e:
        error(f"Failed to query Gradle property '{property_path}', received error: {e}")
        return None

def get_app_details(build_path):
    # Get package name from Gradle
    package_name = get_gradle_project_property(build_path, "android.namespace")

    # Get activity from the manifest (try root level, then app/ subdir)
    activity_name = get_manifest_activity(os.path.join(build_path, "AndroidManifest.xml"))
    if not activity_name:
        activity_name = get_manifest_activity(
            os.path.join(build_path, "app", "AndroidManifest.xml"))

    # Resolve relative activity name
    if activity_name and activity_name.startswith('.') and package_name:
        activity_name = package_name + activity_name

    return package_name, activity_name


# Get app details
package_name, activity_name = get_app_details(args.build_path)
if not package_name:
    die("Failed to retrieve the package name of the app")
if not activity_name:
    die("Failed to retrieve the main activity name of the app")

start_cmd = [*adb_argv, "shell", "am", "start", "-n", f"{package_name}/{activity_name}"]

# On Windows the am-start cmdline length is limited, so forward only Qt/test env vars there.
# TODO: allow explicit filtering of which env vars to forward (e.g. a regex flag).
if sys.platform == "win32":
    env_items = [(key, value) for key, value in os.environ.items()
                 if key.startswith("QT_") or key.startswith("QTEST_")]
else:
    env_items = list(os.environ.items())

# Skip the flag entirely when there is nothing to forward.
env_vars = "\t".join(f"{key}={value}" for key, value in env_items)
if env_vars:
    encoded_env_vars = base64.b64encode(env_vars.encode()).decode()
    start_cmd += ["-e", "extraenvvars", encoded_env_vars]

# Get app arguments
if remaining_args:
    start_cmd += ["-e", "applicationArguments", shlex.quote(' '.join(remaining_args))]

# Get formatted time from device
start_timestamp = ""
try:
    start_timestamp = subprocess.check_output(f"{adb} shell \"date +'%Y-%m-%d %H:%M:%S.%3N'\"",
                                              shell=True).decode().strip()
except Exception as e:
    die(f"Failed to get formatted time from the device, received error: {e}")

try:
    subprocess.run(start_cmd, check=True)
except Exception as e:
    die(f"Failed to start the app {package_name}, received error: {e}")

# Wait for the app to start and retrieve its pid
start_timeout = 5
time_limit = time.time() + start_timeout
pid = None
while pid is None:
    if time.time() > time_limit:
        die(f"Couldn't retrieve the app's PID within {start_timeout} seconds")
    time.sleep(0.5)
    try:
        pidof_output = subprocess.check_output(f"{adb} shell pidof {package_name}", shell=True)
        pid = pidof_output.decode().strip().split()[0]
    except subprocess.CalledProcessError:
        continue

if args.detached:
    sys.exit(0)

# Add a signal handler to stop the app if the script is terminated
interrupted = False
def terminate_app(signum, frame):
    global interrupted
    interrupted = True

signal.signal(signal.SIGINT, terminate_app)

# Show app's logs
logcat_process = None
try:
    format_arg = "-v brief -v color"
    time_arg = f"-T '{start_timestamp}'"
    # escape char and color followed with fatal tag
    fatal_regex = "-e $'^\x1b\\[[0-9]*mF/'"
    pid_regex = f"-e '([ ]*{pid}):'"
    logcat_cmd = f"{adb} shell \"logcat {time_arg} {format_arg} | grep {pid_regex} {fatal_regex}\""
    logcat_process = subprocess.Popen(logcat_cmd, shell=True)
except Exception as e:
    die(f"Failed to get logcat for the app {package_name}, received error: {e}")

# Monitor the app's pid
try:
    while not interrupted:
        time.sleep(1)
        try:
            pidof_output = subprocess.check_output(f"{adb} shell pidof {package_name}", shell=True)
            pid = pidof_output.decode().strip()
            if not pid:
                status(f"The app \"{package_name}\" has exited")
                break
        except subprocess.CalledProcessError:
            status(f"The app \"{package_name}\" has exited")
            break
finally:
    if logcat_process:
        logcat_process.terminate()

if interrupted:
    try:
        subprocess.Popen(f"{adb} shell am force-stop {package_name}", shell=True)
        status(f"The app \"{package_name}\" with {pid} has been terminated")
    except Exception as e:
        error(f"Failed to terminate the app {package_name}, received error: {e}")
