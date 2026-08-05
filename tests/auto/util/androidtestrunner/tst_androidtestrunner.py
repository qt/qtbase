#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Drives the host androidtestrunner from a single test registered on the
# Android target build. CommandLineTests cover argument parsing without a
# device; DeviceRunTests drive the runner against the testapp APK and
# auto-skip when no device is attached.
#
# All paths are passed in as CLI arguments by the CMakeLists.txt wrapper:
#
#   --runner-bin     path to the host androidtestrunner
#   --testapp-apk    path to the built testapp APK
#   --testapp-aab    path to the built testapp AAB (empty when modern
#                    bundle is not configured)
#   --testapp-dir    android-build-tst_androidtestrunner_testapp directory
#   --testapp-wrapper  runner wrapper script CMake generated for the testapp
#   --cmake          path to the host cmake binary
#   --build-dir      standalone-tests build root
#   --bundletool     path to the bundletool jar (empty when not configured)


import argparse
import contextlib
import glob
import os
import re
import shlex
import shutil
import signal
import subprocess
import sys
import tempfile
import time
import unittest
import uuid


# Mirrors the constants in src/tools/androidtestrunner/main.cpp.
HIGHEST_QTEST_EXITCODE = 127
EXIT_ERROR = 254
EXIT_NOEXITCODE = 253
EXIT_ANR = 252
EXIT_NORESULTS = 251
EXIT_DEVICE_GONE = 250

# Diagnostic patterns the runner emits on stderr.
EXITCODE_RE = re.compile(r'Test exitcode:\s*"\s*(-?\d+)\s*"')
CRASH_MARKERS = (
    "Crash dump",      # resolved header from ndk-stack
    "*** *** ***",     # raw Android debuggerd banner
    "Fatal signal",    # libc fatal-signal line
    "\nbacktrace:",    # debuggerd backtrace section header on its own line
)


def _parse_args():
    # Parse args at module load so @unittest.skipUnless decorators can see them.
    parser = argparse.ArgumentParser(add_help=False)
    for name in ("runner-bin", "testapp-apk", "testapp-aab",
                 "testapp-dir", "testapp-package", "testapp-wrapper",
                 "cmake", "build-dir", "bundletool"):
        parser.add_argument(f"--{name}", default="")
    parser.add_argument("--debug", action="store_true")
    parsed, remaining = parser.parse_known_args()
    sys.argv[:] = sys.argv[:1] + remaining
    return parsed


ARGS = _parse_args()

DEBUG = ARGS.debug


def _require(name, value):
    if not value:
        sys.stderr.write(f"FATAL: --{name} is required\n")
        sys.exit(2)
    return value


def find_adb():
    sdk = os.environ.get("ANDROID_SDK_ROOT") or os.environ.get("ANDROID_HOME")
    if sdk:
        path = os.path.join(sdk, "platform-tools", "adb")
        if os.path.exists(path):
            return path
    return shutil.which("adb")


def has_device():
    adb = find_adb()
    if not adb:
        return False
    try:
        out = subprocess.run([adb, "devices"], capture_output=True,
                             text=True, timeout=15)
    except (FileNotFoundError, subprocess.TimeoutExpired):
        return False
    return any(line.strip().endswith("device")
               for line in out.stdout.splitlines()[1:])


_DEVICE_SDK_VERSION = None

def device_sdk_version():
    global _DEVICE_SDK_VERSION
    if _DEVICE_SDK_VERSION is not None:
        return _DEVICE_SDK_VERSION
    adb = find_adb()
    if not adb:
        return 0  # adb may appear on a later call; don't cache the miss
    try:
        out = subprocess.run([adb, "shell", "getprop", "ro.build.version.sdk"],
                             capture_output=True, text=True, timeout=15)
        _DEVICE_SDK_VERSION = int(out.stdout.strip())
    except (FileNotFoundError, subprocess.TimeoutExpired, ValueError):
        return 0  # transient failure; retry on the next call
    return _DEVICE_SDK_VERSION


@contextlib.contextmanager
def fake_adb(serials=("emulator-fake-1",), hang_on=None):
    """Fake adb that lists the given serials; pass hang_on='cmd' to make
    the script sleep forever when that token appears in argv."""
    fd, path = tempfile.mkstemp(prefix="atr-fake-adb-", suffix=".sh")
    device_lines = "".join(f"{s}\\tdevice\\n" for s in serials)
    hang_branch = ""
    if hang_on:
        hang_branch = (f'  elif [ "$a" = "{hang_on}" ]; then\n'
                       "    sleep 999\n")
    with os.fdopen(fd, "w") as f:
        f.write(
            "#!/bin/sh\n"
            'for a in "$@"; do\n'
            '  if [ "$a" = "devices" ]; then\n'
            f'    printf "List of devices attached\\n{device_lines}"\n'
            "    exit 0\n"
            f"{hang_branch}"
            "  fi\n"
            "done\n"
            "exit 0\n"
        )
    os.chmod(path, 0o755)
    try:
        yield path
    finally:
        os.unlink(path)


def write_minimal_manifest(dirpath):
    """Write a minimal AndroidManifest.xml under dirpath; the runner's
    QXmlStreamReader pass only needs <activity> to populate g_options."""
    os.makedirs(dirpath, exist_ok=True)
    with open(os.path.join(dirpath, "AndroidManifest.xml"), "w") as f:
        f.write('<?xml version="1.0"?>\n'
                '<manifest><application>'
                '<activity android:name=".MainActivity"/>'
                '</application></manifest>\n')


@contextlib.contextmanager
def fake_make(name="make"):
    """Tempdir-hosted script named `name` that logs its argv to argv.log
    next to it. Yields (script_path, log_path)."""
    tmpdir = tempfile.mkdtemp(prefix="atr-fake-make-")
    log_path = os.path.join(tmpdir, "argv.log")
    script_path = os.path.join(tmpdir, name)
    with open(script_path, "w") as f:
        f.write(
            "#!/bin/sh\n"
            f"printf '%s\\n' \"$*\" >> {shlex.quote(log_path)}\n"
            "exit 0\n"
        )
    os.chmod(script_path, 0o755)
    try:
        yield script_path, log_path
    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)


@contextlib.contextmanager
def adb_pidof_fail_wrapper(real_adb):
    """adb proxy that fails `shell pidof -s` after evict() while letting
    `adb devices` pass through to the real adb. Drives the runner into the
    isRunning() "3 retries fail, devices list intact" branch."""
    tmpdir = tempfile.mkdtemp(prefix="atr-adb-pidof-")
    wrapper = os.path.join(tmpdir, "adb")
    sentinel = os.path.join(tmpdir, "evicted")
    with open(wrapper, "w") as f:
        f.write(
            "#!/bin/sh\n"
            f'REAL={shlex.quote(real_adb)}\n'
            f'SENTINEL={shlex.quote(sentinel)}\n'
            'ARGS=" $* "\n'
            'if [ -f "$SENTINEL" ]; then\n'
            '    case "$ARGS" in\n'
            '        *" shell pidof -s "*) exit 1 ;;\n'
            '    esac\n'
            'fi\n'
            'exec "$REAL" "$@"\n'
        )
    os.chmod(wrapper, 0o755)
    def evict():
        open(sentinel, "w").close()
    try:
        yield wrapper, evict
    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)


@contextlib.contextmanager
def adb_full_eviction_wrapper(real_adb):
    """adb proxy that empties `devices` and fails every shell/run-as call after
    evict(). Used to simulate disconnect that happens after the test exited."""
    tmpdir = tempfile.mkdtemp(prefix="atr-adb-fullevict-")
    wrapper = os.path.join(tmpdir, "adb")
    sentinel = os.path.join(tmpdir, "evicted")
    with open(wrapper, "w") as f:
        f.write(
            "#!/bin/sh\n"
            f'REAL={shlex.quote(real_adb)}\n'
            f'SENTINEL={shlex.quote(sentinel)}\n'
            'ARGS=" $* "\n'
            'if [ -f "$SENTINEL" ]; then\n'
            '    case "$ARGS" in\n'
            '        *" devices "*)\n'
            "            printf 'List of devices attached\\n'\n"
            '            exit 0\n'
            '            ;;\n'
            '        *) exit 1 ;;\n'
            '    esac\n'
            'fi\n'
            'exec "$REAL" "$@"\n'
        )
    os.chmod(wrapper, 0o755)
    def evict():
        open(sentinel, "w").close()
    try:
        yield wrapper, evict
    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)


@contextlib.contextmanager
def adb_eviction_wrapper(real_adb):
    """adb proxy that empties `devices` and fails the runner's pidof liveness
    probe after evict(), simulating a mid-test disconnect. Yields
    (wrapper_path, evict)."""
    tmpdir = tempfile.mkdtemp(prefix="atr-adb-evict-")
    wrapper = os.path.join(tmpdir, "adb")
    sentinel = os.path.join(tmpdir, "evicted")
    with open(wrapper, "w") as f:
        f.write(
            "#!/bin/sh\n"
            f'REAL={shlex.quote(real_adb)}\n'
            f'SENTINEL={shlex.quote(sentinel)}\n'
            'ARGS=" $* "\n'
            'if [ -f "$SENTINEL" ]; then\n'
            '    case "$ARGS" in\n'
            '        *" devices "*)\n'
            "            printf 'List of devices attached\\n'\n"
            '            exit 0\n'
            '            ;;\n'
            '        *" shell pidof "*)\n'
            '            exit 1\n'
            '            ;;\n'
            '    esac\n'
            'fi\n'
            'exec "$REAL" "$@"\n'
        )
    os.chmod(wrapper, 0o755)
    def evict():
        open(sentinel, "w").close()
    try:
        yield wrapper, evict
    finally:
        shutil.rmtree(tmpdir, ignore_errors=True)


@unittest.skipIf(sys.platform == "win32",
                 "fake adb/make fixtures rely on /bin/sh and POSIX exec bits")
class CommandLineTests(unittest.TestCase):
    """Static checks of the runner's command-line parsing. No device needed."""

    def run_runner(self, args, timeout=30, extra_env=None):
        runner = _require("runner-bin", ARGS.runner_bin)
        argv = [runner] + args
        if DEBUG:
            print("+ " + " ".join(shlex.quote(a) for a in argv), flush=True)
        # Strip host serial so fake adb's advertised serial wins, unless the
        # caller wants to assert env-var precedence explicitly.
        env = {k: v for k, v in os.environ.items()
               if k not in ("ANDROID_SERIAL", "ANDROID_DEVICE_SERIAL")}
        if extra_env:
            env.update(extra_env)
        return subprocess.run(argv, env=env, capture_output=True, text=True, timeout=timeout)

    def test_help_lists_main_options(self):
        proc = self.run_runner(["--help"])
        self.assertEqual(proc.returncode, 0, proc.stderr)
        for needle in ("--apk", "--aab", "--path", "--make"):
            self.assertIn(needle, proc.stdout)

    def test_single_dash_long_option_is_not_recognised(self):
        # Only --name is supported; -name must reach the test as a positional arg.
        proc = self.run_runner(["-help"])
        self.assertNotEqual(proc.returncode, 0)

    def test_unknown_option_returns_exit_error(self):
        # Parse errors must exit with EXIT_ERROR, not the default 1.
        proc = self.run_runner(["--no-such-option"])
        self.assertEqual(proc.returncode, EXIT_ERROR,
            f"unknown option must yield EXIT_ERROR, got {proc.returncode}: {proc.stderr!r}")
        self.assertIn("Unknown option '--no-such-option'", proc.stderr)

    def test_unknown_option_with_required_args_still_rejected(self):
        # Typo'd --paht must not be silently forwarded as a test arg.
        proc = self.run_runner(["--path", "/tmp", "--apk", "/tmp/foo.apk",
                                "--make", "true", "--paht", "/tmp/typo"])
        self.assertEqual(proc.returncode, EXIT_ERROR)
        self.assertIn("Unknown option '--paht'", proc.stderr)

    def test_no_args_fails(self):
        proc = self.run_runner([])
        self.assertEqual(proc.returncode, EXIT_ERROR)
        self.assertIn("--path", proc.stderr)
        self.assertIn("--apk", proc.stderr)

    def test_apk_and_aab_conflict_fails(self):
        proc = self.run_runner(["--path", "/tmp",
                                "--apk", "/tmp/foo.apk",
                                "--aab", "/tmp/foo.aab"])
        self.assertEqual(proc.returncode, EXIT_ERROR)
        self.assertIn("Only one of --apk or --aab", proc.stderr)

    def test_apk_specified_twice_fails(self):
        # QCommandLineParser stores both values; without the size>1 check the
        # runner would silently use the second one.
        proc = self.run_runner(["--path", "/tmp", "--make", "true",
                                "--apk", "/tmp/a.apk",
                                "--apk", "/tmp/b.apk"])
        self.assertEqual(proc.returncode, EXIT_ERROR)
        self.assertIn("--apk specified", proc.stderr)

    def test_aab_specified_twice_fails(self):
        proc = self.run_runner(["--path", "/tmp", "--make", "true",
                                "--aab", "/tmp/a.aab",
                                "--aab", "/tmp/b.aab"])
        self.assertEqual(proc.returncode, EXIT_ERROR)
        self.assertIn("--aab specified", proc.stderr)

    def test_aab_without_bundletool_fails(self):
        proc = self.run_runner(["--path", "/tmp", "--make", "true",
                                "--aab", "/tmp/foo.aab"])
        self.assertEqual(proc.returncode, EXIT_ERROR)
        self.assertIn("--bundletool", proc.stderr)

    def test_missing_make_fails(self):
        proc = self.run_runner(["--path", "/tmp", "--apk", "/tmp/foo.apk"])
        self.assertEqual(proc.returncode, EXIT_ERROR)
        self.assertIn("--make", proc.stderr)

    def test_accepts_test_args_without_double_dash(self):
        # CI wrappers append QTest's CLI without a `--` separator;
        # see main.cpp::splitOwnAndTestArgs.
        proc = self.run_runner(["--path", "/tmp", "--apk", "/tmp/foo.apk",
                                "--make", "true", "--timeout", "60",
                                "-o", "/tmp/foo.xml,xml", "-o", "-,txt"])
        self.assertNotIn("Unknown option", proc.stderr)

    def test_own_option_after_test_args_is_still_ours(self):
        # CMake appends -import before --timeout, forwarding ours made QTest exit 1 (QTBUG-148805).
        proc = self.run_runner(["--path", "/tmp", "--apk", "/tmp/foo.apk", "--make", "true",
                                "-import", "/tmp/imports", "--timeout", "abc"])
        self.assertEqual(proc.returncode, EXIT_ERROR,
            f"--timeout after a test arg was forwarded instead of parsed: {proc.stderr!r}")
        self.assertIn("positive integer", proc.stderr)

    def test_own_value_option_after_test_args_still_takes_effect(self):
        # Not just parsed: a value-taking flag behind a test arg must still drive the run.
        with fake_make("make") as (make_path, log), fake_adb() as adb, \
                tempfile.TemporaryDirectory(prefix="atr-order-") as tmp:
            write_minimal_manifest(tmp)
            apk = os.path.join(tmp, "stub.apk")
            open(apk, "w").close()
            self.run_runner(["--path", tmp, "--apk", apk, "--adb", adb, "--timeout", "1",
                             "-import", "/tmp/a", "--make", make_path])
            self.assertTrue(os.path.exists(log),
                "--make behind a test arg was forwarded to the test instead of honoured")

    def test_verbose_prints_execute_lines(self):
        # --verbose makes execCommand echo each spawned command to stdout.
        proc = self.run_runner(["--verbose", "--path", "/tmp",
                                "--apk", "/tmp/foo.apk", "--make", "true"])
        self.assertIn("Execute true", proc.stdout)

    def test_no_verbose_omits_execute_lines(self):
        # Without --verbose the runner stays quiet about spawned commands.
        proc = self.run_runner(["--path", "/tmp",
                                "--apk", "/tmp/foo.apk", "--make", "true"])
        self.assertNotIn("Execute ", proc.stdout)

    def test_timeout_kills_long_running_command(self):
        # --timeout caps every spawned process. Forcing make to outrun the
        # timeout must kill it within the budget instead of hanging.
        start = time.monotonic()
        proc = self.run_runner(["--timeout", "1", "--path", "/tmp",
                                "--apk", "/tmp/foo.apk", "--make", "sleep 10"])
        elapsed = time.monotonic() - start
        self.assertLess(elapsed, 5,
                        f"runner did not honor --timeout (took {elapsed:.1f}s)")
        self.assertEqual(proc.returncode, EXIT_ERROR)
        self.assertIn("timed out", proc.stderr)

    def test_invalid_timeout_value_fails(self):
        # Non-numeric --timeout silently coerced to 0 would make every spawned
        # process time out immediately; reject it up front instead.
        proc = self.run_runner(["--timeout", "abc", "--path", "/tmp",
                                "--apk", "/tmp/foo.apk", "--make", "true"])
        self.assertEqual(proc.returncode, EXIT_ERROR)
        self.assertIn("timeout", proc.stderr.lower())

    def test_zero_timeout_value_fails(self):
        # --timeout 0 would time every spawned process out immediately.
        proc = self.run_runner(["--timeout", "0", "--path", "/tmp",
                                "--apk", "/tmp/foo.apk", "--make", "true"])
        self.assertEqual(proc.returncode, EXIT_ERROR)
        self.assertIn("positive integer", proc.stderr)

    def test_option_matching_is_case_sensitive(self):
        # QCommandLineParser is case sensitive — upper-case spellings of known
        # options should be treated as unknown.
        self.assertNotEqual(self.run_runner(["--PATH", "/tmp"]).returncode, 0)

    def test_apk_missing_value_fails(self):
        # --apk at the end of argv has no value; the splitter rejects it
        # rather than letting it swallow the next flag.
        proc = self.run_runner(["--path", "/tmp", "--make", "true", "--apk"])
        self.assertEqual(proc.returncode, EXIT_ERROR)
        self.assertIn("apk", proc.stderr.lower(),
                      f"expected an --apk diagnostic in stderr, got: {proc.stderr!r}")
        self.assertIn("requires a value", proc.stderr)

    def test_default_timeout_kills_hanging_adb_command(self):
        # adb (a host tool, not the build/test) gets QProcess's default
        # timeout (~30s) so a wedged adb call is bounded even without --timeout.
        with tempfile.TemporaryDirectory(prefix="atr-adb-hang-") as tmp, \
                fake_adb(hang_on="getprop") as adb:
            write_minimal_manifest(tmp)
            apk = os.path.join(tmp, "stub.apk")
            open(apk, "w").close()
            start = time.monotonic()
            proc = self.run_runner([
                "--path", tmp, "--apk", apk,
                "--make", "true", "--adb", adb,
            ], timeout=60)
            elapsed = time.monotonic() - start
        # QProcess default is 30s; allow extra headroom for slow CI before flaking.
        self.assertLess(elapsed, 60,
                        f"hanging adb not killed in time ({elapsed:.1f}s)")
        self.assertEqual(proc.returncode, EXIT_ERROR)
        self.assertIn("timed out", proc.stderr)

    # The runner only checks that the APK file exists, so a zero-byte stub
    # is enough for CommandLineTests that don't actually install.

    def _run_with_stub_apk(self, build_path, adb_path, *,
                           make="true", extra=()):
        apk = os.path.join(build_path, "stub.apk")
        open(apk, "w").close()
        return self.run_runner([
            "--path", build_path, "--apk", apk, "--make", make,
            "--adb", adb_path, "--timeout", "1",
            *extra,
        ])

    def test_manifest_found_in_build_dir(self):
        with tempfile.TemporaryDirectory(prefix="atr-manifest-") as tmp, \
                fake_adb() as adb:
            write_minimal_manifest(tmp)
            proc = self._run_with_stub_apk(tmp, adb)
            self.assertNotIn("Unable to find AndroidManifest.xml", proc.stderr,
                             proc.stderr)

    def test_manifest_found_under_app_subdir(self):
        with tempfile.TemporaryDirectory(prefix="atr-manifest-") as tmp, \
                fake_adb() as adb:
            write_minimal_manifest(os.path.join(tmp, "app"))
            proc = self._run_with_stub_apk(tmp, adb)
            self.assertNotIn("Unable to find AndroidManifest.xml", proc.stderr,
                             proc.stderr)

    def test_no_manifest_fails_clearly(self):
        with tempfile.TemporaryDirectory(prefix="atr-manifest-") as tmp, \
                fake_adb() as adb:
            proc = self._run_with_stub_apk(tmp, adb)
            self.assertEqual(proc.returncode, EXIT_ERROR)
            self.assertIn("Unable to find AndroidManifest.xml", proc.stderr)

    def test_manifest_explicit_path_overrides_search(self):
        # --manifest must short-circuit processAndroidManifest()'s build-dir
        # search; a manifest in a sibling directory is enough.
        with tempfile.TemporaryDirectory(prefix="atr-manifest-explicit-") as tmp, \
                tempfile.TemporaryDirectory(prefix="atr-manifest-side-") as side, \
                fake_adb() as adb:
            # No manifest in tmp; only in side.
            write_minimal_manifest(side)
            manifest = os.path.join(side, "AndroidManifest.xml")
            proc = self._run_with_stub_apk(tmp, adb,
                                           extra=("--manifest", manifest))
            self.assertNotIn("Unable to find AndroidManifest.xml", proc.stderr,
                             proc.stderr)

    def test_manifest_explicit_nonexistent_fails(self):
        # Must fail clearly instead of falling back to the build-dir search.
        with tempfile.TemporaryDirectory(prefix="atr-manifest-nox-") as tmp, fake_adb() as adb:
            proc = self._run_with_stub_apk(tmp, adb,
                extra=("--manifest", "/no/such/AndroidManifest.xml"))
            self.assertEqual(proc.returncode, EXIT_ERROR)
            self.assertIn("does not exist", proc.stderr)

    def test_bundletool_nonexistent_fails(self):
        # Must fail upfront instead of deferring to a 'java -jar /no/such.jar' error.
        with tempfile.TemporaryDirectory(prefix="atr-bt-nox-") as tmp, fake_adb() as adb:
            write_minimal_manifest(tmp)
            aab = os.path.join(tmp, "stub.aab")
            open(aab, "w").close()
            proc = self.run_runner(["--path", tmp, "--aab", aab, "--make", "true",
                                    "--bundletool", "/no/such.jar",
                                    "--adb", adb, "--timeout", "1"])
            self.assertEqual(proc.returncode, EXIT_ERROR)
            self.assertIn("does not exist", proc.stderr)

    def test_activity_option_is_accepted(self):
        with tempfile.TemporaryDirectory(prefix="atr-activity-") as tmp, fake_adb() as adb:
            write_minimal_manifest(tmp)
            proc = self._run_with_stub_apk(tmp, adb,
                extra=("--activity", "com.example.MyActivity"))
            self.assertNotIn("Unknown option", proc.stderr)
            self.assertNotIn("requires a value", proc.stderr)

    def _assert_make_argv(self, basename, *, expect_install_root):
        with fake_make(basename) as (make_path, log), \
                fake_adb() as adb, \
                tempfile.TemporaryDirectory(prefix="atr-mk-") as tmp:
            write_minimal_manifest(tmp)
            self._run_with_stub_apk(tmp, adb, make=make_path)
            self.assertTrue(os.path.exists(log),
                            f"fake make ({basename}) was never invoked")
            with open(log) as f:
                logged = f.read()
        if expect_install_root:
            self.assertIn("INSTALL_ROOT=", logged,
                          f"expected INSTALL_ROOT for {basename!r}, got {logged!r}")
        else:
            self.assertNotIn("INSTALL_ROOT=", logged,
                             f"unexpected INSTALL_ROOT for {basename!r}: {logged!r}")

    def test_install_root_appended_for_make(self):
        self._assert_make_argv("make", expect_install_root=True)

    def test_install_root_appended_for_gmake(self):
        self._assert_make_argv("gmake", expect_install_root=True)

    def test_install_root_appended_for_nmake(self):
        self._assert_make_argv("nmake", expect_install_root=True)

    def test_install_root_appended_for_mingw32_make(self):
        self._assert_make_argv("mingw32-make", expect_install_root=True)

    def test_install_root_appended_for_jom(self):
        self._assert_make_argv("jom", expect_install_root=True)

    def test_install_root_skipped_for_cmake(self):
        self._assert_make_argv("cmake", expect_install_root=False)

    def test_install_root_skipped_for_ninja(self):
        self._assert_make_argv("ninja", expect_install_root=False)

    def test_skip_install_root_suppresses_suffix(self):
        # --skip-install-root must beat the GNU make family auto-suffix.
        with fake_make("make") as (make_path, log), \
                fake_adb() as adb, \
                tempfile.TemporaryDirectory(prefix="atr-skip-") as tmp:
            write_minimal_manifest(tmp)
            self._run_with_stub_apk(tmp, adb, make=make_path, extra=("--skip-install-root",))
            self.assertTrue(os.path.exists(log), "fake make was never invoked")
            with open(log) as f:
                logged = f.read()
        self.assertNotIn("INSTALL_ROOT=", logged, f"--skip-install-root not honored: {logged!r}")

    def test_apk_file_must_exist(self):
        # The runner must trip on a phantom APK path before reaching install.
        with tempfile.TemporaryDirectory(prefix="atr-noapk-") as tmp, fake_adb() as adb:
            write_minimal_manifest(tmp)
            ghost = os.path.join(tmp, "does-not-exist.apk")
            proc = self.run_runner(["--path", tmp, "--apk", ghost, "--make", "true",
                                    "--adb", adb, "--timeout", "1"])
            self.assertEqual(proc.returncode, EXIT_ERROR)
            self.assertIn("No package", proc.stderr)

    def _run_for_serial_check(self, adb_path, extra=(), extra_env=None):
        with tempfile.TemporaryDirectory(prefix="atr-serial-") as tmp:
            write_minimal_manifest(tmp)
            apk = os.path.join(tmp, "stub.apk")
            open(apk, "w").close()
            return self.run_runner([
                "--path", tmp, "--apk", apk, "--make", "true",
                "--adb", adb_path, "--timeout", "1",
                *extra,
            ], extra_env=extra_env)

    def test_no_devices_fails(self):
        with fake_adb(serials=()) as adb:
            proc = self._run_for_serial_check(adb)
            self.assertEqual(proc.returncode, EXIT_ERROR)
            self.assertIn("No connected devices", proc.stderr)

    def test_multiple_devices_without_serial_fails(self):
        with fake_adb(serials=("dev-A", "dev-B")) as adb:
            proc = self._run_for_serial_check(adb)
            self.assertEqual(proc.returncode, EXIT_ERROR)
            self.assertIn("Multiple devices connected", proc.stderr)

    def test_single_device_pinned(self):
        with fake_adb(serials=("dev-A",)) as adb:
            proc = self._run_for_serial_check(adb)
            for needle in ("Multiple devices", "No connected devices",
                           "with serial '"):
                self.assertNotIn(needle, proc.stderr, proc.stderr)

    def test_explicit_serial_pins_one_of_many(self):
        # --serial must pin the named one of several attached devices.
        with fake_adb(serials=("dev-A", "dev-B", "dev-C")) as adb:
            proc = self._run_for_serial_check(adb, extra=("--serial", "dev-B"))
            for needle in ("Multiple devices", "No connected devices",
                           "with serial '"):
                self.assertNotIn(needle, proc.stderr, proc.stderr)

    def test_explicit_serial_not_in_devices_fails(self):
        with fake_adb(serials=("dev-A",)) as adb:
            proc = self._run_for_serial_check(adb,
                                              extra=("--serial", "missing"))
            self.assertEqual(proc.returncode, EXIT_ERROR)
            self.assertIn("with serial 'missing'", proc.stderr)

    def test_serial_flag_overrides_android_serial_env(self):
        # --serial dev-B wins over ANDROID_SERIAL=dev-A; runner picks dev-B.
        with fake_adb(serials=("dev-A", "dev-B")) as adb:
            proc = self._run_for_serial_check(adb,
                extra=("--serial", "dev-B"),
                extra_env={"ANDROID_SERIAL": "dev-A"})
            for needle in ("Multiple devices", "No connected devices",
                           "with serial '"):
                self.assertNotIn(needle, proc.stderr, proc.stderr)

    def test_android_serial_env_is_used(self):
        # ANDROID_SERIAL pins one of multiple devices without --serial.
        with fake_adb(serials=("dev-A", "dev-B")) as adb:
            proc = self._run_for_serial_check(adb,
                extra_env={"ANDROID_SERIAL": "dev-A"})
            for needle in ("Multiple devices", "No connected devices",
                           "with serial '"):
                self.assertNotIn(needle, proc.stderr, proc.stderr)

    def test_android_device_serial_env_is_fallback(self):
        # ANDROID_DEVICE_SERIAL is honored when ANDROID_SERIAL is absent.
        with fake_adb(serials=("dev-A", "dev-B")) as adb:
            proc = self._run_for_serial_check(adb,
                extra_env={"ANDROID_DEVICE_SERIAL": "dev-B"})
            for needle in ("Multiple devices", "No connected devices",
                           "with serial '"):
                self.assertNotIn(needle, proc.stderr, proc.stderr)

    def test_android_serial_beats_android_device_serial(self):
        # Both env vars set: ANDROID_SERIAL wins.
        with fake_adb(serials=("dev-A", "dev-B")) as adb:
            proc = self._run_for_serial_check(adb,
                extra_env={"ANDROID_SERIAL": "dev-A",
                           "ANDROID_DEVICE_SERIAL": "missing"})
            for needle in ("Multiple devices", "No connected devices",
                           "with serial '"):
                self.assertNotIn(needle, proc.stderr, proc.stderr)

    def test_adb_devices_failure_reports_clearly(self):
        # When `adb devices` itself fails (returns non-zero), the runner must
        # bail with a clear message instead of mis-reporting "no devices".
        fd, path = tempfile.mkstemp(prefix="atr-broken-adb-", suffix=".sh")
        with os.fdopen(fd, "w") as f:
            f.write("#!/bin/sh\nexit 1\n")
        os.chmod(path, 0o755)
        try:
            proc = self._run_for_serial_check(path)
            self.assertEqual(proc.returncode, EXIT_ERROR)
            self.assertIn("Failed to query connected devices", proc.stderr)
        finally:
            os.unlink(path)

    def _run_with_fake_gradlew(self, gradlew_body):
        """Build a fake build-tree with a gradlew script, run the runner, return proc."""
        tmp = tempfile.mkdtemp(prefix="atr-gradle-")
        try:
            write_minimal_manifest(tmp)
            apk = os.path.join(tmp, "stub.apk")
            open(apk, "w").close()
            gradlew = os.path.join(tmp, "gradlew")
            with open(gradlew, "w") as f:
                f.write(gradlew_body)
            os.chmod(gradlew, 0o755)
            with fake_adb() as adb:
                proc = self.run_runner([
                    "--path", tmp, "--apk", apk, "--make", "true",
                    "--adb", adb,
                ])
            return proc
        finally:
            shutil.rmtree(tmp, ignore_errors=True)

    def test_gradlew_nonzero_exit_is_warned(self):
        # A failed gradlew property query must warn (not silently fail later).
        proc = self._run_with_fake_gradlew(
            "#!/bin/sh\necho 'broken' >&2\nexit 1\n")
        self.assertNotEqual(proc.returncode, 0)
        self.assertIn("gradlew", proc.stderr)

    @unittest.skipIf(sys.platform == "win32", "SIGINT delivery semantics differ on Windows")
    def test_early_sigint_before_install_calls_exit(self):
        # SIGINT before isPackageInstalled is set must take the _exit(EXIT_ERROR)
        # branch in sigHandler (no graceful cleanup needed since nothing installed).
        with tempfile.TemporaryDirectory(prefix="atr-early-sig-") as tmp, \
                fake_adb() as adb:
            write_minimal_manifest(tmp)
            apk = os.path.join(tmp, "stub.apk")
            open(apk, "w").close()
            argv = [_require("runner-bin", ARGS.runner_bin),
                    "--path", tmp, "--apk", apk, "--make", "sleep 5",
                    "--adb", adb]
            env = {k: v for k, v in os.environ.items()
                   if k not in ("ANDROID_SERIAL", "ANDROID_DEVICE_SERIAL")}
            proc = subprocess.Popen(argv, env=env,
                                    stdout=subprocess.DEVNULL,
                                    stderr=subprocess.DEVNULL)
            time.sleep(2)
            if proc.poll() is not None:
                self.fail(f"runner exited before SIGINT (returncode {proc.returncode})")
            proc.send_signal(signal.SIGINT)
            proc.wait(timeout=15)
        self.assertEqual(proc.returncode, EXIT_ERROR,
            f"early SIGINT should exit with EXIT_ERROR, got {proc.returncode}")

    @unittest.skipIf(sys.platform == "win32", "SIGTERM delivery semantics differ on Windows")
    def test_early_sigterm_before_install_calls_exit(self):
        with tempfile.TemporaryDirectory(prefix="atr-early-term-") as tmp, \
                fake_adb() as adb:
            write_minimal_manifest(tmp)
            apk = os.path.join(tmp, "stub.apk")
            open(apk, "w").close()
            argv = [_require("runner-bin", ARGS.runner_bin),
                    "--path", tmp, "--apk", apk, "--make", "sleep 5",
                    "--adb", adb]
            env = {k: v for k, v in os.environ.items()
                   if k not in ("ANDROID_SERIAL", "ANDROID_DEVICE_SERIAL")}
            proc = subprocess.Popen(argv, env=env,
                                    stdout=subprocess.DEVNULL,
                                    stderr=subprocess.DEVNULL)
            time.sleep(2)
            if proc.poll() is not None:
                self.fail(f"runner exited before SIGTERM (returncode {proc.returncode})")
            proc.send_signal(signal.SIGTERM)
            proc.wait(timeout=15)
        self.assertEqual(proc.returncode, EXIT_ERROR,
            f"early SIGTERM should exit with EXIT_ERROR, got {proc.returncode}")

    def test_apk_path_without_apk_or_aab_extension_fails(self):
        with tempfile.TemporaryDirectory(prefix="atr-extn-") as tmp, fake_adb() as adb:
            write_minimal_manifest(tmp)
            bogus = os.path.join(tmp, "stub.bin")
            open(bogus, "w").close()
            proc = self.run_runner(["--path", tmp, "--apk", bogus, "--make", "true",
                                    "--adb", adb, "--timeout", "1"])
            self.assertEqual(proc.returncode, EXIT_ERROR)
            self.assertIn(".apk or .aab", proc.stderr)

    def test_apk_equals_form_is_accepted(self):
        with tempfile.TemporaryDirectory(prefix="atr-eq-") as tmp, fake_adb() as adb:
            write_minimal_manifest(tmp)
            apk = os.path.join(tmp, "stub.apk")
            open(apk, "w").close()
            proc = self.run_runner(["--path", tmp, f"--apk={apk}", "--make", "true",
                                    "--adb", adb, "--timeout", "1"])
            self.assertNotIn("Unknown option", proc.stderr)
            self.assertNotIn("requires a value", proc.stderr)

    def test_help_lists_exit_codes(self):
        proc = self.run_runner(["--help"])
        self.assertEqual(proc.returncode, 0, proc.stderr)
        for code in ("250", "251", "252", "253", "254"):
            self.assertIn(code, proc.stdout, f"exit code {code} missing from --help")

    def test_concurrent_different_serials_both_proceed(self):
        # Per-serial lock: two runners pinned to different serials must each
        # acquire their own lock and not block on each other.
        with fake_adb(serials=("dev-A",)) as adb_a, \
                fake_adb(serials=("dev-B",)) as adb_b, \
                tempfile.TemporaryDirectory(prefix="atr-multi-") as tmp:
            write_minimal_manifest(tmp)
            apk = os.path.join(tmp, "stub.apk")
            open(apk, "w").close()
            base = ["--path", tmp, "--apk", apk, "--make", "true", "--timeout", "1"]
            env = {k: v for k, v in os.environ.items()
                   if k not in ("ANDROID_SERIAL", "ANDROID_DEVICE_SERIAL")}
            start = time.monotonic()
            proc_a = subprocess.run([_require("runner-bin", ARGS.runner_bin), *base,
                                     "--adb", adb_a, "--serial", "dev-A"],
                                    env=env, capture_output=True, text=True, timeout=60)
            proc_b = subprocess.run([_require("runner-bin", ARGS.runner_bin), *base,
                                     "--adb", adb_b, "--serial", "dev-B"],
                                    env=env, capture_output=True, text=True, timeout=60)
            elapsed = time.monotonic() - start
            for p in (proc_a, proc_b):
                self.assertNotIn("Failed to acquire test runner lock", p.stderr,
                                 f"unexpected lock contention: {p.stderr!r}")
            self.assertLess(elapsed, 30,
                f"per-serial isolation regressed; took {elapsed:.1f}s")

class RunnerWrapperScriptTests(unittest.TestCase):
    """Check the runner wrapper script that CMake generates next to a target."""

    @unittest.skipIf(sys.platform == "win32",
                     "the .bat wrapper forwards %*, which keeps the quoting")
    @unittest.skipUnless(ARGS.testapp_wrapper, "runner wrapper script path not passed in")
    def test_wrapper_forwards_arguments_quoted(self):
        # An unquoted $@ word-splits every argument, so ctest re-running a data
        # row like "parseFile:from_file:extended multiplexing" reaches the test
        # as three separate test names. The tool itself quotes correctly, so the
        # wrapper is the only place where this can be lost.
        with open(ARGS.testapp_wrapper) as f:
            lines = [line.strip() for line in f if line.strip()]
        self.assertEqual(lines[-1], '"$@"',
                         f"wrapper does not forward argv verbatim: {lines[-1]!r}")


@unittest.skipUnless(has_device(), "no Android device or emulator attached")
class DeviceRunTests(unittest.TestCase):
    """Drive androidtestrunner against the testapp APK on a real device."""

    PER_RUN_TIMEOUT = 180

    @classmethod
    def setUpClass(cls):
        cls.runner = _require("runner-bin", ARGS.runner_bin)
        cls.apk = _require("testapp-apk", ARGS.testapp_apk)
        cls.build_path = _require("testapp-dir", ARGS.testapp_dir)
        cls.testapp_package = _require("testapp-package", ARGS.testapp_package)
        cls.aab = ARGS.testapp_aab
        cls.bundletool = ARGS.bundletool
        cmake = _require("cmake", ARGS.cmake)
        build_dir = _require("build-dir", ARGS.build_dir)
        # Pre-quoted so QProcess::splitCommand in the runner re-splits correctly;
        # callers below use shlex.split, which strips the outer quotes again.
        cls.make_apk = (f'"{cmake}" --build "{build_dir}" '
                        f'--target tst_androidtestrunner_testapp_make_apk')
        cls.make_aab = (f'"{cmake}" --build "{build_dir}" '
                        f'--target tst_androidtestrunner_testapp_make_aab')
        # Pin the serial once so every spawned androidtestrunner targets the same device.
        # Bail with a clear message instead of running tests that are going to fail.
        cls.serial = cls._select_serial()
        cls._saved_serial = os.environ.get("ANDROID_SERIAL")
        os.environ["ANDROID_SERIAL"] = cls.serial

    @classmethod
    def tearDownClass(cls):
        if cls._saved_serial is None:
            os.environ.pop("ANDROID_SERIAL", None)
        else:
            os.environ["ANDROID_SERIAL"] = cls._saved_serial

    @staticmethod
    def _select_serial():
        for env in ("ANDROID_SERIAL", "ANDROID_DEVICE_SERIAL"):
            if value := os.environ.get(env):
                return value
        adb = find_adb()
        if not adb:
            raise unittest.SkipTest("adb not found on PATH or via ANDROID_SDK_ROOT")
        out = subprocess.run([adb, "devices"], capture_output=True, text=True, timeout=15)
        serials = [line.split("\t", 1)[0] for line in out.stdout.splitlines()[1:]
                   if line.endswith("\tdevice")]
        if not serials:
            raise unittest.SkipTest("no Android device or emulator attached")
        if len(serials) > 1:
            raise unittest.SkipTest(
                f"multiple devices attached ({', '.join(serials)}); "
                f"set ANDROID_SERIAL to pick one")
        return serials[0]

    def tearDown(self):
        # Best-effort uninstall: a crashed runner can leave the package behind.
        adb = find_adb()
        if adb:
            subprocess.run([adb, "uninstall", self.testapp_package],
                           capture_output=True, text=True, timeout=15)
        for stale in self._lock_files():
            try:
                os.unlink(stale)
            except OSError:
                pass

    def _argv(self, *, use_aab=False, extra=(), make=None):
        # make=None uses the real cmake target; pass "true" to skip rebuild.
        if use_aab:
            pkg = ["--aab", self.aab, "--bundletool", self.bundletool,
                   "--make", make if make is not None else self.make_aab]
        else:
            pkg = ["--apk", self.apk,
                   "--make", make if make is not None else self.make_apk]
        return [self.runner, "--path", self.build_path, *pkg,
                "--timeout", str(self.PER_RUN_TIMEOUT), *extra]

    def invoke(self, test_args=None, *, extra_env=None, scenario=None,
               extra_runner_args=(), cwd=None):
        env = os.environ.copy()
        if scenario:
            env["QT_TESTRUNNER_SCENARIO"] = scenario
        if extra_env:
            env.update(extra_env)
        argv = self._argv(extra=extra_runner_args)
        if test_args:
            argv += ["--", *test_args]
        if DEBUG:
            print("+ " + " ".join(shlex.quote(a) for a in argv), flush=True)
        return subprocess.run(argv, env=env, capture_output=True,
                              text=True, cwd=cwd,
                              timeout=self.PER_RUN_TIMEOUT + 60)

    def assert_abnormal(self, returncode):
        # Crash / no-exit-code / result-pull failures land above 127. Reject
        # EXIT_ANR here: it's exercised explicitly by test_anr_blocked_ui_thread.
        self.assertGreater(returncode, HIGHEST_QTEST_EXITCODE,
                           f"expected abnormal exit (>127), got {returncode}")
        self.assertNotEqual(returncode, EXIT_ANR)

    def test_scenario_pass(self):
        proc = self.invoke(scenario="pass", test_args=["runScenario"])
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)

    def test_scenario_fail(self):
        proc = self.invoke(scenario="fail", test_args=["runScenario"])
        self.assertEqual(proc.returncode, 1, proc.stdout + proc.stderr)

    def test_scenario_skip(self):
        proc = self.invoke(scenario="skip", test_args=["runScenario"])
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
        # A clean pass also returns 0; assert QSKIP actually fired.
        self.assertRegex(proc.stdout + proc.stderr,
            r"SKIP\s*:\s*tst_AndroidTestRunnerTestApp::runScenario")

    def test_scenario_crash(self):
        proc = self.invoke(scenario="crash", test_args=["runScenario"])
        self.assertEqual(proc.returncode, EXIT_NOEXITCODE,
            f"got {proc.returncode}\n{proc.stdout}{proc.stderr}")

    def test_scenario_qfatal(self):
        self.assert_abnormal(self.invoke(
            scenario="qfatal", test_args=["runScenario"]).returncode)

    def test_scenario_crash_in_init(self):
        proc = self.invoke(scenario="crash_in_init", test_args=["runScenario"])
        self.assertEqual(proc.returncode, EXIT_NOEXITCODE,
            f"got {proc.returncode}\n{proc.stdout}{proc.stderr}")

    def test_scenario_crash_before_main(self):
        # Crashes in a static initializer before main().
        proc = self.invoke(scenario="crash_before_main", test_args=["alwaysPasses"])
        self.assertEqual(proc.returncode, EXIT_NOEXITCODE,
            f"got {proc.returncode}\n{proc.stdout}{proc.stderr}")

    def test_test_arg_with_space_stays_one_argument(self):
        # qt-testrunner re-runs a failing data row as a single argument, e.g.
        # "parseFile:from_file:extended multiplexing". The quoting has to survive
        # adb, the device shell, the am start extra and QProcess::splitCommand()
        # in androidjnimain.cpp, otherwise QTest sees one test name per word.
        proc = self.invoke(test_args=["no such function"])
        out = proc.stdout + proc.stderr
        self.assertIn("Function not found: no such function", out, out)

    @unittest.skipUnless(
        ARGS.testapp_aab and ARGS.bundletool,
        "modern bundle build (-DQT_USE_ANDROID_MODERN_BUNDLE=ON + bundletool) not configured")
    def test_aab_install_and_run(self):
        # bundletool builds a .apks, install-apks pushes it, then am start runs the testapp.
        argv = self._argv(use_aab=True) + ["--", "alwaysPasses"]
        proc = subprocess.run(argv, env=os.environ.copy(),
                              capture_output=True, text=True,
                              timeout=self.PER_RUN_TIMEOUT + 60)
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
        # A returncode of 0 alone doesn't prove the test ran (a runner that
        # exited 0 before launch would also pass). Assert PASS output landed.
        self.assertRegex(proc.stdout + proc.stderr,
            r"PASS\s*:\s*tst_AndroidTestRunnerTestApp::alwaysPasses")

    # Per-serial QLockFile in tempdir guards concurrent runs.

    @classmethod
    def _lock_files(cls):
        # Wildcard the user prefix; match only this device's serial.
        return glob.glob(os.path.join(tempfile.gettempdir(),
                                      f"androidtestrunner-*-{cls.serial}.lock"))

    def _wait_for_lock_file(self, timeout):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            files = self._lock_files()
            if files:
                return files[0]
            time.sleep(0.1)
        return None

    @unittest.skipIf(sys.platform == "win32",
                     "uses --make 'true' (POSIX builtin) and SIGKILL semantics")
    def test_killed_runner_does_not_block_next_one(self):
        # SIGKILL leaves the lock file behind; QLockFile's PID-based stale
        # detection (default 30s) must reclaim it before the second runner
        # times out.
        for stale in self._lock_files():
            try:
                os.unlink(stale)
            except OSError:
                pass

        # Pre-build so the lock file shows up before _wait_for_lock_file expires.
        subprocess.run(shlex.split(self.make_apk), check=True, capture_output=True, timeout=300)

        # Hold proc1 in a deterministic pre-test sleep so it still owns the lock
        # by the time we kill it; without this it can finish before SIGKILL.
        hold = ("--pre-test-adb-command", "shell sleep 20")
        argv = self._argv(make="true", extra=hold) + ["--", "alwaysPasses"]
        proc1 = subprocess.Popen(argv, env=os.environ.copy(),
                                 stdout=subprocess.DEVNULL,
                                 stderr=subprocess.DEVNULL)
        try:
            lock_path = self._wait_for_lock_file(timeout=60.0)
            assert lock_path is not None, \
                "first runner never created its lock file"
            proc1.kill()
            proc1.wait(timeout=10.0)
            self.assertTrue(os.path.exists(lock_path),
                "lock file unexpectedly disappeared after SIGKILL")
        finally:
            if proc1.poll() is None:
                proc1.kill()
                proc1.wait()

        proc2 = self.invoke(test_args=["alwaysPasses"])
        self.assertEqual(proc2.returncode, 0, proc2.stdout + proc2.stderr)

    def test_select_passing_function(self):
        proc = self.invoke(test_args=["alwaysPasses"])
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)

    def test_select_failing_function(self):
        proc = self.invoke(test_args=["alwaysFails"])
        self.assertEqual(proc.returncode, 1, proc.stdout + proc.stderr)

    def test_select_skipping_function(self):
        # QSKIP is not a failure: the runner must return 0.
        proc = self.invoke(test_args=["alwaysSkips"])
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)

    def test_select_multiple_functions_all_clean(self):
        proc = self.invoke(test_args=[
            "alwaysPasses", "alwaysSkips",
            "test_function_with_underscores_and_digits_123",
        ])
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)

    def test_select_multiple_functions_with_failure(self):
        proc = self.invoke(test_args=["alwaysPasses", "alwaysFails"])
        self.assertEqual(proc.returncode, 1, proc.stdout + proc.stderr)

    def test_data_tag_shapes_pass(self):
        tags = ["rowOne", "row with spaces", "row-with-dashes",
                "row_with_underscores", "row.with.dots", "row/with/slashes",
                "special!@#$%^&*()"]
        proc = self.invoke(test_args=[f"dataDriven:{t}" for t in tags])
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)

    def test_data_tag_with_double_quotes(self):
        # A literal double quote in a test arg must reach the testapp intact.
        proc = self.invoke(test_args=['dataDriven:row"with"quotes'])
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)

    def test_data_tag_failing_row(self):
        proc = self.invoke(test_args=["dataDriven:rowTwo"])
        self.assertEqual(proc.returncode, 1, proc.stdout + proc.stderr)

    def test_data_tag_all_rows_default(self):
        # No tag -> all rows; rowTwo fails so exit code is 1.
        proc = self.invoke(test_args=["dataDriven"])
        self.assertEqual(proc.returncode, 1, proc.stdout + proc.stderr)

    def test_both_prefixes_forwarded_together(self):
        proc = self.invoke(test_args=["verifyEnvVarQt", "verifyEnvVarQtest"],
                           extra_env={
            "QT_TESTRUNNER_PROBE": "qt_value",
            "QTEST_TESTRUNNER_PROBE": "qtest_value",
        })
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)

    def test_unprefixed_env_var_not_forwarded(self):
        # An unprefixed variable on the host must not reach the app.
        proc = self.invoke(test_args=["verifyEnvVarLeak"],
                           extra_env={"TESTRUNNER_LEAK_PROBE": "leaked"})
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)

    def test_env_var_value_shapes_survive(self):
        proc = self.invoke(test_args=["verifyEnvVarValueShapes"], extra_env={
            "QT_TESTRUNNER_PROBE_PLAIN": "hello-world_123",
            "QT_TESTRUNNER_PROBE_SPECIAL":
                'special!@#$%^&*()_+-=,.<>?[]{}|/\\\'"',
            "QT_TESTRUNNER_PROBE_SPACES": "hello world with spaces",
        })
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)

    def test_multiple_env_vars_with_spaces_survive(self):
        # Stress the tab-separator wire protocol: two QT_ vars both with
        # spaces. A receiver that still split on space (the pre-fix behaviour)
        # would corrupt the first value and mis-key the second.
        proc = self.invoke(test_args=["verifyEnvVarFullValueWithSpaces"],
                           extra_env={
            "QT_TESTRUNNER_PROBE": "hello world with spaces",
            "QT_TESTRUNNER_SECOND_PROBE": "another value with spaces",
        })
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)

    def test_env_var_value_with_literal_tab(self):
        proc = self.invoke(test_args=["verifyEnvVarTabInValue"],
                           extra_env={
            "QT_TESTRUNNER_PROBE_TAB": "before\tafter",
        })
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)

    def _pulled(self, path):
        self.assertTrue(os.path.exists(path), f"runner did not pull {path}")
        with open(path, encoding="utf-8") as f:
            return f.read()

    def test_pulls_multiple_formats_simultaneously(self):
        # qt-testrunner.py asks for all four formats in one shot.
        with tempfile.TemporaryDirectory(prefix="tst_atr_") as tmp:
            xml = os.path.join(tmp, "out.xml")
            junit = os.path.join(tmp, "out.junit.xml")
            txt = os.path.join(tmp, "out.txt")
            proc = self.invoke(test_args=["-o", f"{xml},xml",
                                          "-o", f"{junit},junitxml",
                                          "-o", f"{txt},txt",
                                          "-o", "-,txt", "alwaysPasses"])
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            self.assertIn("<TestCase", self._pulled(xml))
            self.assertIn("<testsuite", self._pulled(junit))
            self.assertIn("alwaysPasses", self._pulled(txt))

    def test_old_style_format_flag(self):
        # `-xml` is the deprecated single-format spelling; must still produce parseable XML.
        with tempfile.TemporaryDirectory(prefix="tst_atr_") as tmp:
            xml = os.path.join(tmp, "old.xml")
            proc = self.invoke(test_args=["-o", xml, "-xml", "alwaysPasses"])
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            self.assertIn("<TestCase", self._pulled(xml))

    def test_explicit_file_output_without_format(self):
        # -o foo.txt without ,fmt used to break the runner via an empty tail target.
        with tempfile.TemporaryDirectory(prefix="tst_atr_explicit_") as tmp:
            out = os.path.join(tmp, "explicit.out")
            proc = self.invoke(test_args=["-o", out, "alwaysPasses"])
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            self.assertIn("alwaysPasses", self._pulled(out))

    def test_output_long_form_alias(self):
        # `--output file,fmt` is accepted in test args as an alias for `-o`.
        with tempfile.TemporaryDirectory(prefix="tst_atr_long_") as tmp:
            xml = os.path.join(tmp, "long.xml")
            proc = self.invoke(test_args=["--output", f"{xml},xml", "alwaysPasses"])
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            self.assertIn("<TestCase", self._pulled(xml))

    @staticmethod
    def _reported_exit_code(output):
        m = EXITCODE_RE.search(output)
        return int(m.group(1)) if m else None

    def test_runner_reports_test_exit_code_for_pass(self):
        proc = self.invoke(scenario="pass", test_args=["runScenario"])
        self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
        self.assertEqual(self._reported_exit_code(proc.stdout + proc.stderr), 0)

    def test_runner_reports_test_exit_code_for_fail(self):
        proc = self.invoke(scenario="fail", test_args=["runScenario"])
        self.assertEqual(proc.returncode, 1, proc.stdout + proc.stderr)
        self.assertEqual(self._reported_exit_code(proc.stdout + proc.stderr), 1)

    def test_streams_live_qtest_output(self):
        # tail -F mirrors files/stdout.txt; PASS, FAIL! and Totals must reach the host.
        proc = self.invoke(test_args=["alwaysPasses", "alwaysFails"])
        self.assertEqual(proc.returncode, 1, proc.stdout + proc.stderr)
        output = proc.stdout + proc.stderr
        self.assertIn("Start testing of tst_AndroidTestRunnerTestApp", output)
        self.assertRegex(output,
            r"PASS\s*:\s*tst_AndroidTestRunnerTestApp::alwaysPasses\(\)")
        self.assertRegex(output,
            r"FAIL!\s*:\s*tst_AndroidTestRunnerTestApp::alwaysFails\(\)")
        self.assertIn("Totals:", output)

    def test_dash_output_does_not_leak_file_in_cwd(self):
        # `-o -,<fmt>` means "stream to stdout"; the runner must not also
        # pull files/stdout.<fmt> from the device and write it to the host CWD.
        with tempfile.TemporaryDirectory(prefix="tst_atr_cwd_") as cwd:
            proc = self.invoke(test_args=["-o", "-,txt", "alwaysPasses"],
                               cwd=cwd)
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            leftovers = [n for n in os.listdir(cwd) if n.startswith("stdout.")]
            self.assertEqual(leftovers, [],
                f"runner left files in CWD: {leftovers}")

    def _assert_native_crash_trace(self, output):
        self.assertIn("BEGIN logcat dump", output,
            "runner did not dump logcat on a crash")
        self.assertTrue(any(m in output for m in CRASH_MARKERS),
            "no native crash marker in:\n" + output[-3000:])

    def test_stack_trace_on_crash_in_init(self):
        # Crash in initTestCase: abnormal exit, logcat dumped, native backtrace surfaced.
        proc = self.invoke(scenario="crash_in_init", test_args=["runScenario"])
        self.assert_abnormal(proc.returncode)
        self._assert_native_crash_trace(proc.stdout + proc.stderr)

    def test_crash_stack_for_late_crash_with_show_logcat(self):
        # Same expectation for a mid-test crash; --show-logcat must keep the diagnostics.
        proc = self.invoke(scenario="crash", test_args=["runScenario"],
                           extra_runner_args=["--show-logcat"])
        self.assert_abnormal(proc.returncode)
        self._assert_native_crash_trace(proc.stdout + proc.stderr)

    # ndk-stack

    @staticmethod
    def _crash_dump(output):
        # ndk-stack output starts with "********** Crash dump:"; the runner
        # falls back to "BEGIN crash dump" when ndk-stack didn't run.
        for marker in ("********** Crash dump", "BEGIN crash dump"):
            idx = output.find(marker)
            if idx != -1:
                return output[idx:]
        return ""

    def test_ndk_stack_resolves_native_crash(self):
        # Verify the crash dump shows resolved frames, not raw hex offsets.
        ndk_root = (os.environ.get("ANDROID_NDK_ROOT")
                    or os.environ.get("ANDROID_NDK_HOME"))
        if not ndk_root or not os.path.exists(
                os.path.join(ndk_root, "ndk-stack")):
            self.skipTest("ANDROID_NDK_ROOT/ndk-stack not available")
        proc = self.invoke(scenario="crash", test_args=["runScenario"],
                           extra_runner_args=["--show-logcat"])
        self.assert_abnormal(proc.returncode)
        dump = self._crash_dump(proc.stdout + proc.stderr)
        self.assertIn("********** Crash dump", dump,
            f"no ndk-stack header:\n{dump}")
        self.assertIn("tst_AndroidTestRunnerTestApp::runScenario", dump,
            f"crashing frame not resolved to a C++ symbol:\n{dump}")

    def test_show_logcat_emits_logcat_output(self):
        # The runner wraps its logcat dump in BEGIN/END markers; with -v brief
        # the body looks like "I/Tag(<pid>): ...". Both should appear when
        # --show-logcat is set, and neither without it.
        plain = self.invoke(scenario="pass", test_args=["runScenario"])
        self.assertEqual(plain.returncode, 0, plain.stdout + plain.stderr)
        self.assertNotIn("BEGIN logcat dump", plain.stdout + plain.stderr,
                         "logcat dump emitted without --show-logcat")

        verbose = self.invoke(scenario="pass", test_args=["runScenario"],
                              extra_runner_args=("--show-logcat",))
        self.assertEqual(verbose.returncode, 0, verbose.stdout + verbose.stderr)
        combined = verbose.stdout + verbose.stderr
        self.assertIn("BEGIN logcat dump", combined,
                      "--show-logcat did not emit the logcat dump wrapper")
        self.assertIn("END logcat dump", combined)
        # Lines can be ANSI-coloured (`logcat -v color`) unless QTEST_ENVIRONMENT=ci;
        # Android 9 emits 256-color escapes like \x1b[38;5;40m, so allow ';'.
        self.assertRegex(combined,
                         r"(?m)^(?:\x1b\[[\d;]+m)?[VDIWEF]/\S+\s*\(\s*\d+\)\s*:",
                         "no brief-format logcat lines in --show-logcat output")

    def test_pre_test_adb_command_failure_aborts_with_exit_error(self):
        # A failing --pre-test-adb-command must propagate as EXIT_ERROR.
        proc = self.invoke(
            scenario="pass", test_args=["runScenario"],
            extra_runner_args=("--pre-test-adb-command", "shell exit 1"))
        self.assertEqual(proc.returncode, EXIT_ERROR,
            f"got {proc.returncode}\n{proc.stdout}{proc.stderr}")
        self.assertIn("pre test ADB command", proc.stdout + proc.stderr)

    def test_ndk_stack_missing_env_warns_and_keeps_dump(self):
        # With no ndk-stack available, warn and still print the raw crash dump.
        ndk = tempfile.mkdtemp(prefix="atr-ndk-empty-")
        try:
            proc = self.invoke(scenario="crash", test_args=["runScenario"],
                               extra_runner_args=("--show-logcat",),
                               extra_env={"ANDROID_NDK_ROOT": ndk})
            self.assert_abnormal(proc.returncode)
            output = proc.stdout + proc.stderr
            self.assertIn("ndk-stack path not provided", output,
                "missing-ndk warning not printed")
            self.assertTrue(any(m in output for m in CRASH_MARKERS),
                f"no crash markers in dump:\n{output[-3000:]}")
        finally:
            shutil.rmtree(ndk, ignore_errors=True)

    def test_pre_test_adb_command_runs_before_test(self):
        # Drop a sentinel file via --pre-test-adb-command and verify it
        # lands on the device. /data/local/tmp/ is writable by the adb
        # shell user across both user and userdebug builds.
        adb = find_adb()
        assert adb is not None
        marker = f"/data/local/tmp/atr-pretest-{uuid.uuid4().hex}"
        subprocess.run([adb, "shell", "rm", "-f", marker], timeout=10)
        try:
            proc = self.invoke(
                scenario="pass", test_args=["runScenario"],
                extra_runner_args=("--pre-test-adb-command",
                                   f"shell touch {marker}"))
            self.assertEqual(proc.returncode, 0, proc.stdout + proc.stderr)
            got = subprocess.run([adb, "shell", "ls", marker],
                                 capture_output=True, text=True, timeout=10)
            self.assertEqual(got.returncode, 0,
                f"--pre-test-adb-command did not create {marker}: {got.stderr!r}")
        finally:
            subprocess.run([adb, "shell", "rm", "-f", marker], timeout=10)

    @staticmethod
    def _screen_center(adb):
        # Parses "Physical size: WxH" from `wm size`; (500, 500) fallback.
        out = subprocess.run([adb, "shell", "wm", "size"],
                             capture_output=True, text=True, timeout=10)
        m = re.search(r"(\d+)x(\d+)", out.stdout)
        if m:
            return int(m.group(1)) // 2, int(m.group(2)) // 2
        return 500, 500

    @classmethod
    def _wait_for_foreground(cls, adb, package, timeout=90):
        # `dumpsys window windows` carried mCurrentFocus pre-Android 16, since
        # then it lives under `dumpsys window displays`; `dumpsys window`
        # spans both layouts.
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            out = subprocess.run([adb, "shell", "dumpsys", "window"],
                                 capture_output=True, text=True, timeout=10)
            for line in out.stdout.splitlines():
                if "mCurrentFocus" in line and package in line:
                    return True
            time.sleep(0.5)
        return False

    @staticmethod
    def _wait_for_pid(adb, package, timeout=45):
        # Faster than foreground polling; only needs the process to exist.
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            out = subprocess.run([adb, "shell", "pidof", package],
                                 capture_output=True, text=True, timeout=5)
            if out.stdout.strip():
                return True
            time.sleep(0.2)
        return False

    @staticmethod
    def _wait_for_pid_gone(adb, package, timeout=90):
        deadline = time.monotonic() + timeout
        while time.monotonic() < deadline:
            out = subprocess.run([adb, "shell", "pidof", package],
                                 capture_output=True, text=True, timeout=5)
            if not out.stdout.strip():
                return True
            time.sleep(0.2)
        return False

    def test_anr_blocked_ui_thread(self):
        # The anrBlockUi slot stalls the UI thread; a single tap once it's
        # foreground makes ActivityManager log "ANR in <pkg>" ~5s later.
        if device_sdk_version() < 30:
            # On API<30 (Android 10 and earlier) QtActivity init partly fails
            # (OnBackInvokedCallback is API 33+) and runOnAndroidMainThread
            # doesn't reliably block the UI thread, so no ANR fires.
            self.skipTest("ANR test relies on Qt UI-thread blocking that requires Android 11+")
        adb = find_adb()
        assert adb is not None, "adb required for ANR test"
        argv = self._argv(extra=("--show-logcat",)) + ["--", "anrBlockUi"]
        # tempfile not PIPE: --show-logcat overruns the default pipe buffer.
        with tempfile.TemporaryFile(mode="w+", prefix="anr-") as logf:
            proc = subprocess.Popen(argv, env=os.environ.copy(),
                                    stdout=logf, stderr=subprocess.STDOUT)
            input_proc = None
            try:
                if not self._wait_for_foreground(adb, self.testapp_package):
                    proc.kill()
                    logf.seek(0)
                    self.fail(
                        f"testapp never came to foreground:\n{logf.read()}")
                cx, cy = self._screen_center(adb)
                # A single tap is enough on some Android versions, but on others
                # one tap may not provoke ANR. Spam taps in a device-side loop
                # so the input dispatcher accumulates undelivered events.
                input_proc = subprocess.Popen(
                    [adb, "shell",
                     f"for i in $(seq 1 30); do input tap {cx} {cy}; sleep 1; done"],
                    stdout=subprocess.DEVNULL,
                    stderr=subprocess.DEVNULL)
                proc.wait(timeout=self.PER_RUN_TIMEOUT + 60)
            except subprocess.TimeoutExpired:
                proc.kill()
                logf.seek(0)
                self.fail(f"runner did not exit:\n{logf.read()}")
            finally:
                if input_proc is not None and input_proc.poll() is None:
                    input_proc.kill()
                    input_proc.wait(timeout=10)
            logf.seek(0)
            output = logf.read()
        self.assertEqual(proc.returncode, EXIT_ANR,
            f"expected EXIT_ANR (252), got {proc.returncode}\n{output}")

    @unittest.skipIf(sys.platform == "win32", "ndk-stack shim uses /bin/sh")
    def test_ndk_stack_explicit_path_is_used(self):
        # Shim ndk-stack with a sentinel printer; the explicit path must be invoked.
        sentinel = "ndk-stack-shim-sentinel"
        fd, path = tempfile.mkstemp(prefix="atr-shim-ndkstack-", suffix=".sh")
        with os.fdopen(fd, "w") as f:
            f.write(f"#!/bin/sh\ncat\nprintf '%s\\n' '{sentinel}'\nexit 0\n")
        os.chmod(path, 0o755)
        try:
            proc = self.invoke(scenario="crash", test_args=["runScenario"],
                               extra_runner_args=("--ndk-stack", path, "--show-logcat"))
            self.assert_abnormal(proc.returncode)
            self.assertIn(sentinel, proc.stdout + proc.stderr,
                "explicit --ndk-stack path was not invoked")
        finally:
            os.unlink(path)

    @unittest.skipIf(sys.platform == "win32", "ndk-stack shim uses /bin/sh")
    def test_ndk_stack_auto_discovery_from_env(self):
        # With no --ndk-stack but $ANDROID_NDK_ROOT/ndk-stack present, the runner
        # must invoke the env-derived path on a crash.
        sentinel = "ndk-stack-auto-sentinel"
        ndk_root = tempfile.mkdtemp(prefix="atr-ndk-auto-")
        path = os.path.join(ndk_root, "ndk-stack")
        with open(path, "w") as f:
            f.write(f"#!/bin/sh\ncat\nprintf '%s\\n' '{sentinel}'\nexit 0\n")
        os.chmod(path, 0o755)
        try:
            proc = self.invoke(scenario="crash", test_args=["runScenario"],
                               extra_runner_args=("--show-logcat",),
                               extra_env={"ANDROID_NDK_ROOT": ndk_root})
            self.assert_abnormal(proc.returncode)
            self.assertIn(sentinel, proc.stdout + proc.stderr,
                "auto-discovered ndk-stack was not invoked")
        finally:
            shutil.rmtree(ndk_root, ignore_errors=True)

    @unittest.skipIf(sys.platform == "win32", "SIGINT delivery semantics differ on Windows")
    def test_sigint_aborts_cleanly(self):
        # SIGINT mid-run must exit non-zero, tear down the adb-tail child,
        # and still uninstall the test package.
        adb = find_adb()
        assert adb is not None, "adb required for sigint test"
        subprocess.run([adb, "uninstall", self.testapp_package],
                       capture_output=True, timeout=15)
        argv = self._argv() + ["--", "anrBlockUi"]
        proc = subprocess.Popen(argv, env=os.environ.copy(),
                                stdout=subprocess.DEVNULL,
                                stderr=subprocess.DEVNULL)
        try:
            if not self._wait_for_pid(adb, self.testapp_package, timeout=60):
                proc.kill()
                self.fail("testapp never started; cannot exercise SIGINT path")
            proc.send_signal(signal.SIGINT)
            proc.wait(timeout=60)
        finally:
            if proc.poll() is None:
                proc.kill()
                proc.wait()
        self.assertNotEqual(proc.returncode, 0,
            f"SIGINT should produce non-zero exit, got {proc.returncode}")
        out = subprocess.run([adb, "shell", "pm", "path", self.testapp_package],
                             capture_output=True, text=True, timeout=10)
        self.assertEqual(out.stdout.strip(), "",
            f"runner did not uninstall after SIGINT: {out.stdout!r}")

    def test_crash_dump_printed_when_ndk_stack_missing(self):
        # waitForStarted() fails for a missing ndk-stack path; the dump must still print.
        proc = self.invoke(scenario="crash", test_args=["runScenario"],
                           extra_runner_args=("--ndk-stack", "/no/such/ndk-stack",
                                              "--show-logcat"))
        self.assert_abnormal(proc.returncode)
        output = proc.stdout + proc.stderr
        self.assertIn("failed to run ndk-stack", output)
        self.assertIn("BEGIN crash dump", output, "unsymbolicated dump suppressed")

    @unittest.skipIf(sys.platform == "win32", "ndk-stack shim uses /bin/sh")
    def test_crash_dump_printed_when_ndk_stack_exits_nonzero(self):
        # ndk-stack drains stdin and exits 1; the dump must still print.
        fd, path = tempfile.mkstemp(prefix="atr-bad-ndkstack-", suffix=".sh")
        with os.fdopen(fd, "w") as f:
            f.write("#!/bin/sh\ncat > /dev/null\nexit 1\n")
        os.chmod(path, 0o755)
        try:
            proc = self.invoke(scenario="crash", test_args=["runScenario"],
                               extra_runner_args=("--ndk-stack", path, "--show-logcat"))
            self.assert_abnormal(proc.returncode)
            output = proc.stdout + proc.stderr
            self.assertTrue(
                any(m in output for m in ("BEGIN crash dump", "********** Crash dump")),
                f"crash dump suppressed when ndk-stack exited 1:\n{output[-3000:]}")
            # Empty ndk-stack stdout must not overwrite the raw dump body.
            dump = self._crash_dump(output)
            self.assertTrue(any(m in dump for m in CRASH_MARKERS),
                f"empty ndk-stack output dropped the raw dump:\n{dump}")
        finally:
            os.unlink(path)

    @unittest.skipIf(sys.platform == "win32", "POSIX exec bits")
    def test_crash_dump_printed_when_ndk_stack_not_executable(self):
        # No execute bit fails QProcess::start; the raw dump must still print.
        fd, path = tempfile.mkstemp(prefix="atr-noexec-ndkstack-")
        os.close(fd)
        os.chmod(path, 0o644)
        try:
            proc = self.invoke(scenario="crash", test_args=["runScenario"],
                               extra_runner_args=("--ndk-stack", path, "--show-logcat"))
            self.assert_abnormal(proc.returncode)
            output = proc.stdout + proc.stderr
            self.assertIn("failed to run ndk-stack", output)
            self.assertIn("BEGIN crash dump", output,
                f"unsymbolicated dump suppressed:\n{output[-3000:]}")
        finally:
            os.unlink(path)

    @unittest.skipIf(sys.platform == "win32",
                     "uses --pre-test-adb-command sleep / POSIX exec bits")
    def test_concurrent_same_serial_second_runner_waits_for_turn(self):
        # lock() blocks until the holder releases, so a contended runner queues
        # behind the first and then runs, rather than failing fast.
        for stale in self._lock_files():
            try:
                os.unlink(stale)
            except OSError:
                pass
        subprocess.run(shlex.split(self.make_apk), check=True,
                       capture_output=True, timeout=300)
        hold_seconds = 10
        hold = ("--pre-test-adb-command", f"shell sleep {hold_seconds}")
        argv1 = self._argv(make="true", extra=hold) + ["--", "alwaysPasses"]
        proc1 = subprocess.Popen(argv1, env=os.environ.copy(),
                                 stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
        try:
            lock_path = self._wait_for_lock_file(timeout=30.0)
            self.assertIsNotNone(lock_path, "first runner never created its lock file")
            # proc1 holds the lock and sleeps hold_seconds before releasing, so
            # the contention the second runner must queue behind is real.
            self.assertIsNone(proc1.poll(),
                "first runner exited before the second could contend the lock")
            start2 = time.monotonic()
            argv2 = self._argv(make="true") + ["--", "alwaysPasses"]
            proc2 = subprocess.run(argv2, env=os.environ.copy(), capture_output=True,
                                   text=True, timeout=self.PER_RUN_TIMEOUT + 60)
            elapsed2 = time.monotonic() - start2
            # Succeeding while proc1 held the lock proves it queued and waited; a
            # fail-fast lock would have returned EXIT_ERROR right away.
            self.assertEqual(proc2.returncode, 0,
                f"queued runner should succeed after waiting, got {proc2.returncode}\n"
                f"{proc2.stdout}{proc2.stderr}")
            self.assertNotIn("Failed to acquire test runner lock", proc2.stderr,
                "queued runner failed fast instead of waiting for the lock")
            self.assertGreater(elapsed2, hold_seconds - 2,
                f"queued runner returned in {elapsed2:.1f}s; it did not wait out "
                f"proc1's {hold_seconds}s hold")
            proc1.wait(timeout=30)
            self.assertEqual(proc1.returncode, 0)
        finally:
            if proc1.poll() is None:
                proc1.kill()
                proc1.wait()

    def test_pull_results_failure_yields_exit_noresults(self):
        # drop_results wipes result files at atexit; pullResults reads empty.
        with tempfile.TemporaryDirectory(prefix="tst_atr_nores_") as tmp:
            xml = os.path.join(tmp, "result.xml")
            proc = self.invoke(scenario="drop_results",
                               test_args=["-o", f"{xml},xml", "-o", "-,txt", "runScenario"])
            self.assertEqual(proc.returncode, EXIT_NORESULTS,
                f"got {proc.returncode}\n{proc.stdout}{proc.stderr}")

    def test_missing_exit_code_yields_exit_noexitcode(self):
        # drop_exit_code unlinks qtest_last_exit_code at atexit -> EXIT_NOEXITCODE.
        proc = self.invoke(scenario="drop_exit_code", test_args=["runScenario"])
        self.assertEqual(proc.returncode, EXIT_NOEXITCODE,
            f"got {proc.returncode}\n{proc.stdout}{proc.stderr}")

    @unittest.skipIf(sys.platform == "win32",
                     "adb_eviction_wrapper relies on /bin/sh and POSIX exec bits")
    def test_device_disconnect_yields_exit_device_gone(self):
        # Evict via a wrapped adb that empties `devices` and fails `shell ps`.
        real_adb = find_adb()
        assert real_adb is not None, "real adb required to proxy through"
        with adb_eviction_wrapper(real_adb) as (wrapper, evict):
            argv = self._argv(extra=("--adb", wrapper)) + ["--", "blockForDisconnect"]
            proc = subprocess.Popen(argv, env=os.environ.copy(),
                                    stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
            try:
                if not self._wait_for_pid(real_adb, self.testapp_package, timeout=60):
                    proc.kill()
                    self.fail("testapp never started; cannot evict")
                evict()
                proc.wait(timeout=self.PER_RUN_TIMEOUT)
            finally:
                if proc.poll() is None:
                    proc.kill()
                    proc.wait()
        self.assertEqual(proc.returncode, EXIT_DEVICE_GONE,
            f"got {proc.returncode}, expected EXIT_DEVICE_GONE ({EXIT_DEVICE_GONE})")

    @unittest.skipIf(sys.platform == "win32",
                     "adb_pidof_fail_wrapper relies on /bin/sh and POSIX exec bits")
    def test_pidof_failures_with_device_present_skip_device_gone(self):
        # Force `shell pidof -s` to fail while `adb devices` still lists the
        # serial. The 3-retry loop in isRunning() must fall through without
        # promoting the failure to EXIT_DEVICE_GONE.
        real_adb = find_adb()
        assert real_adb is not None, "real adb required to proxy through"
        with adb_pidof_fail_wrapper(real_adb) as (wrapper, evict):
            argv = self._argv(extra=("--adb", wrapper, "--timeout", "15")) \
                + ["--", "anrBlockUi"]
            # DEVNULL, not PIPE: an ANR dumps a large logcat and we don't read
            # the pipes until after wait(), which would deadlock the runner on
            # a full pipe buffer. We only care about the exit code here.
            proc = subprocess.Popen(argv, env=os.environ.copy(),
                                    stdout=subprocess.DEVNULL,
                                    stderr=subprocess.DEVNULL)
            try:
                if not self._wait_for_pid(real_adb, self.testapp_package, timeout=60):
                    proc.kill()
                    self.fail("testapp never started; cannot inject pidof failures")
                evict()
                proc.wait(timeout=self.PER_RUN_TIMEOUT)
            finally:
                if proc.poll() is None:
                    proc.kill()
                    proc.wait()
        self.assertNotEqual(proc.returncode, EXIT_DEVICE_GONE,
            f"runner promoted pidof failure to EXIT_DEVICE_GONE despite the "
            f"device being present (returncode={proc.returncode})")

    @unittest.skipIf(sys.platform == "win32",
                     "adb_full_eviction_wrapper relies on /bin/sh and POSIX exec bits")
    def test_post_test_eviction_yields_exit_device_gone(self):
        # Disconnect happens after waitForFinished returns cleanly. pullResults
        # then fails; the runner must consult adb devices and return
        # EXIT_DEVICE_GONE instead of the vaguer EXIT_NORESULTS.
        real_adb = find_adb()
        assert real_adb is not None, "real adb required to proxy through"
        # A file-backed -o forces pullResults to issue an adb pull that must
        # hit the evicted device, instead of racing the ~600ms exit-code read.
        with adb_full_eviction_wrapper(real_adb) as (wrapper, evict), \
                tempfile.TemporaryDirectory(prefix="tst_atr_evict_") as tmp:
            xml = os.path.join(tmp, "result.xml")
            argv = self._argv(extra=("--adb", wrapper)) + \
                ["--", "-o", f"{xml},xml", "alwaysPasses"]
            # bufsize=1 keeps stderr line-buffered so the marker reaches us as
            # soon as it is written, without a read-ahead buffer delaying it.
            proc = subprocess.Popen(argv, env=os.environ.copy(),
                                    stdout=subprocess.DEVNULL,
                                    stderr=subprocess.PIPE, text=True, bufsize=1)
            try:
                # Anchor eviction to the runner's post-test phase: it logs
                # "Test exitcode:" (qDebug -> stderr) right before pullResults,
                # so evicting then makes the result pull hit the gone device,
                # instead of racing our own pid-gone detection against it.
                evicted = False
                for line in proc.stderr:
                    if "Test exitcode:" in line:
                        evict()
                        evicted = True
                        break
                if not evicted:
                    proc.kill()
                    self.fail("runner never reached post-test phase (no exitcode marker)")
                # Drain stderr and wait, bounded, so a wedged runner can't hang
                # the read on a full pipe.
                try:
                    proc.communicate(timeout=self.PER_RUN_TIMEOUT)
                except subprocess.TimeoutExpired:
                    proc.kill()
                    proc.communicate()
            finally:
                if proc.poll() is None:
                    proc.kill()
                    proc.wait()
        self.assertEqual(proc.returncode, EXIT_DEVICE_GONE,
            f"got {proc.returncode}, expected EXIT_DEVICE_GONE ({EXIT_DEVICE_GONE})")


if __name__ == "__main__":
    unittest.main(failfast=False, verbosity=2)
