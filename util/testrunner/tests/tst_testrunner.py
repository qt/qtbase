#!/usr/bin/env python3
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


import sys
import os
import re
import glob
import subprocess

from subprocess import STDOUT, PIPE
from tempfile   import TemporaryDirectory, mkstemp

MY_NAME          = os.path.basename(__file__)
my_dir           = os.path.dirname(__file__)
testrunner       = os.path.join(my_dir, "..", "qt-testrunner.py")
mock_test        = os.path.join(my_dir, "qt_mock_test.py")
xml_log_template = os.path.join(my_dir, "qt_mock_test-log.xml")

with open(xml_log_template) as f:
    XML_TEMPLATE = f.read()


import unittest

def setUpModule():
    global TEMPDIR
    TEMPDIR = TemporaryDirectory(prefix="tst_testrunner-")

    global EMPTY_FILE
    EMPTY_FILE = os.path.join(TEMPDIR.name, "EMPTY")
    with open(EMPTY_FILE, "w") as f:
        pass

    filename = os.path.join(TEMPDIR.name, "file_1")
    print("setUpModule(): setting up temporary directory and env var"
          " QT_MOCK_TEST_STATE_FILE=" + filename + " and"
          " QT_MOCK_TEST_XML_TEMPLATE_FILE=" + xml_log_template)

    os.environ["QT_MOCK_TEST_STATE_FILE"]        = filename
    os.environ["QT_MOCK_TEST_XML_TEMPLATE_FILE"] = xml_log_template

def tearDownModule():
    print("\ntearDownModule(): Cleaning up temporary directory:",
          TEMPDIR.name)
    del os.environ["QT_MOCK_TEST_STATE_FILE"]
    TEMPDIR.cleanup()


# Helper to run a command and always capture output
def run(args : list, **kwargs):
    if args[0].endswith(".py"):
        # Make sure we run python executables with the same python version.
        # It also helps on Windows, that .py files are not directly executable.
        args = [ sys.executable, *args ]
    if DEBUG:
        print("Running: ", args, flush=True)
    proc = subprocess.run(args, stdout=PIPE, stderr=STDOUT, **kwargs)
    if DEBUG and proc.stdout:
        print(proc.stdout.decode(), flush=True)
    return proc

# Helper to run qt-testrunner.py with proper testing arguments.
def run_testrunner(xml_filename=None, testrunner_args=None,
                   wrapper_script=None, wrapper_args=None,
                   qttest_args=None, env=None):

    args = [ testrunner ]
    if xml_filename:
        args += [ "--parse-xml-testlog", xml_filename ]
    if testrunner_args:
        args += testrunner_args

    if wrapper_script:
        args += [ wrapper_script ]
    if wrapper_args:
        args += wrapper_args

    args += [ mock_test ]
    if qttest_args:
        args += qttest_args

    return run(args, env=env)

# Write the XML_TEMPLATE to filename, replacing the templated results.
def write_xml_log(filename, failure=None, inject_message=None):
    data = XML_TEMPLATE
    if failure is None:
        failure = []
    elif isinstance(failure, str):
        failure = [ failure ]
    # Replace what was asked to fail with "fail"
    for x in failure:
        data = data.replace("{{" + x + "_result}}", "fail")
    # Replace the rest with "pass"
    data = re.sub(r"{{[^}]+}}", "pass", data)
    # Inject possible <Message> tags inside the first <TestFunction>
    if inject_message:
        i = data.index("</TestFunction>")
        data = data[:i] + inject_message + data[i:]
    with open(filename, "w") as f:
        f.write(data)


# Test that qt_mock_test.py behaves well. This is necessary to properly
# test qt-testrunner.
class Test_qt_mock_test(unittest.TestCase):
    def setUp(self):
        state_file = os.environ["QT_MOCK_TEST_STATE_FILE"]
        if os.path.exists(state_file):
            os.remove(state_file)
    def assertProcessCrashed(self, proc):
        if DEBUG:
            print("process returncode is:", proc.returncode)
        self.assertTrue(proc.returncode < 0 or
                        proc.returncode >= 128)

    def test_always_pass(self):
        proc = run([mock_test, "always_pass"])
        self.assertEqual(proc.returncode, 0)
    def test_always_fail(self):
        proc = run([mock_test, "always_fail"])
        self.assertEqual(proc.returncode, 1)
    def test_fail_then_pass_2(self):
        proc = run([mock_test, "fail_then_pass:2"])
        self.assertEqual(proc.returncode, 1)
        proc = run([mock_test, "fail_then_pass:2"])
        self.assertEqual(proc.returncode, 1)
        proc = run([mock_test, "fail_then_pass:2"])
        self.assertEqual(proc.returncode, 0)
    def test_fail_then_pass_1(self):
        proc = run([mock_test, "fail_then_pass:1"])
        self.assertEqual(proc.returncode, 1)
        proc = run([mock_test, "fail_then_pass:1"])
        self.assertEqual(proc.returncode, 0)
    def test_fail_then_pass_many_tests(self):
        proc = run([mock_test, "fail_then_pass:1"])
        self.assertEqual(proc.returncode, 1)
        proc = run([mock_test, "fail_then_pass:2"])
        self.assertEqual(proc.returncode, 1)
        proc = run([mock_test, "fail_then_pass:1"])
        self.assertEqual(proc.returncode, 0)
        proc = run([mock_test, "fail_then_pass:2"])
        self.assertEqual(proc.returncode, 1)
        proc = run([mock_test, "fail_then_pass:2"])
        self.assertEqual(proc.returncode, 0)
    def test_fail_then_crash(self):
        proc = run([mock_test, "fail_then_crash"])
        self.assertEqual(proc.returncode, 1)
        proc = run([mock_test, "fail_then_crash"])
        self.assertProcessCrashed(proc)
    def test_xml_file_is_written(self):
        filename = os.path.join(TEMPDIR.name, "testlog.xml")
        proc = run([mock_test, "-o", filename+",xml"])
        self.assertEqual(proc.returncode, 0)
        self.assertTrue(os.path.exists(filename))
        self.assertGreater(os.path.getsize(filename), 0)
        os.remove(filename)
    # Test it will write an empty XML file if template is empty
    def test_empty_xml_file_is_written(self):
        my_env = {
            **os.environ,
            "QT_MOCK_TEST_XML_TEMPLATE_FILE": EMPTY_FILE
        }
        filename = os.path.join(TEMPDIR.name, "testlog.xml")
        proc = run([mock_test, "-o", filename+",xml"],
                   env=my_env)
        self.assertEqual(proc.returncode, 0)
        self.assertTrue(os.path.exists(filename))
        self.assertEqual(os.path.getsize(filename), 0)
        os.remove(filename)
    def test_crash_cleanly(self):
        proc = run([mock_test],
                   env={ **os.environ, "QT_MOCK_TEST_CRASH_CLEANLY":"1" })
        if DEBUG:
            print("returncode:", proc.returncode)
        self.assertProcessCrashed(proc)


# Find in @path, files that start with @testname and end with @pattern,
# where @pattern is a glob-like string.
def find_test_logs(testname=None, path=None, pattern="-*[0-9].xml"):
    if testname is None:
        testname = os.path.basename(mock_test)
    if path is None:
        path = TEMPDIR.name
    pattern = os.path.join(path, testname + pattern)
    logfiles = glob.glob(pattern)
    if DEBUG:
        print(f"Test ({testname}) logfiles found: ", logfiles)
    return logfiles

# Test regular invocations of qt-testrunner.
class Test_testrunner(unittest.TestCase):
    def setUp(self):
        state_file = os.environ["QT_MOCK_TEST_STATE_FILE"]
        if os.path.exists(state_file):
            os.remove(state_file)
        # The mock_test honors only the XML output arguments, the rest are ignored.
        old_logfiles = find_test_logs(pattern="*.xml")
        for fname in old_logfiles:
            os.remove(os.path.join(TEMPDIR.name, fname))
        self.env = dict(os.environ)
        self.testrunner_args = [ "--log-dir", TEMPDIR.name ]
    def prepare_env(self, run_list=None):
        if run_list is not None:
            self.env['QT_MOCK_TEST_RUN_LIST'] = ",".join(run_list)
    def run2(self):
        return run_testrunner(testrunner_args=self.testrunner_args, env=self.env)

    def test_simple_invocation(self):
        # All tests pass.
        proc = self.run2()
        self.assertEqual(proc.returncode, 0)
    def test_always_pass(self):
        self.prepare_env(run_list=["always_pass"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 0)
    def test_output_files_are_generated(self):
        proc = self.run2()
        xml_output_files = find_test_logs()
        self.assertEqual(len(xml_output_files), 1)
    def test_always_fail(self):
        self.prepare_env(run_list=["always_fail"])
        proc = self.run2()
        # TODO verify that re-runs==max_repeats
        self.assertEqual(proc.returncode, 2)
    def test_flaky_pass_1(self):
        self.prepare_env(run_list=["always_pass,fail_then_pass:1"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 0)
    def test_flaky_pass_5(self):
        self.prepare_env(run_list=["always_pass,fail_then_pass:1,fail_then_pass:5"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 0)
    def test_flaky_fail(self):
        self.prepare_env(run_list=["always_pass,fail_then_pass:6"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 2)
    def test_flaky_pass_fail(self):
        self.prepare_env(run_list=["always_pass,fail_then_pass:1,fail_then_pass:6"])
        proc = self.run2()
        # TODO verify that one func was re-run and passed but the other failed.
        self.assertEqual(proc.returncode, 2)
    def test_initTestCase_fail_crash(self):
        self.prepare_env(run_list=["initTestCase,always_pass"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 3)
    def test_fail_then_crash(self):
        self.prepare_env(run_list=["fail_then_crash"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 3)

    # By testing --no-extra-args, we ensure qt-testrunner works for
    # tst_selftests and the other NON_XML_GENERATING_TESTS.
    def test_no_extra_args_pass(self):
        self.testrunner_args += ["--no-extra-args"]
        proc = self.run2()
        self.assertEqual(proc.returncode, 0)
    def test_no_extra_args_fail(self):
        self.prepare_env(run_list=["always_fail"])
        self.testrunner_args += ["--no-extra-args"]
        proc = self.run2()
        self.assertEqual(proc.returncode, 3)
    def test_no_extra_args_reruns_only_once_1(self):
        self.prepare_env(run_list=["fail_then_pass:1"])
        self.testrunner_args += ["--no-extra-args"]
        proc = self.run2()
        # The 1st rerun PASSed.
        self.assertEqual(proc.returncode, 0)
    def test_no_extra_args_reruns_only_once_2(self):
        self.prepare_env(run_list=["fail_then_pass:2"])
        self.testrunner_args += ["--no-extra-args"]
        proc = self.run2()
        # We never re-run more than once, so the exit code shows FAIL.
        self.assertEqual(proc.returncode, 3)

    # If no XML file is found by qt-testrunner, it is usually considered a
    # CRASH and the whole test is re-run. Even when the return code is zero.
    # It is a PASS only if the test is not capable of XML output (see no_extra_args above).
    def test_no_xml_log_written_pass_crash(self):
        del self.env["QT_MOCK_TEST_XML_TEMPLATE_FILE"]
        self.prepare_env(run_list=["always_pass"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 3)
    # On the 2nd iteration of the full test, both of the tests pass.
    # Still it's a CRASH because no XML file was found.
    def test_no_xml_log_written_fail_then_pass_crash(self):
        del self.env["QT_MOCK_TEST_XML_TEMPLATE_FILE"]
        self.prepare_env(run_list=["always_pass,fail_then_pass:1"])
        proc = self.run2()
        # TODO verify that the whole test has run twice.
        self.assertEqual(proc.returncode, 3)
    # Even after 2 iterations of the full test we still get failures but no XML file,
    # and this is considered a CRASH.
    def test_no_xml_log_written_crash(self):
        del self.env["QT_MOCK_TEST_XML_TEMPLATE_FILE"]
        self.prepare_env(run_list=["fail_then_pass:2"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 3)

    def test_empty_xml_crash_1(self):
        self.env["QT_MOCK_TEST_XML_TEMPLATE_FILE"] = EMPTY_FILE
        self.prepare_env(run_list=["always_pass"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 3)
    def test_empty_xml_crash_2(self):
        self.env["QT_MOCK_TEST_XML_TEMPLATE_FILE"] = EMPTY_FILE
        self.prepare_env(run_list=["always_fail"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 3)

    # test qFatal should be a crash in all cases.
    def test_qfatal_crash_1(self):
        fatal_xml_message = """
            <Message type="qfatal" file="" line="0">
              <DataTag><![CDATA[modal]]></DataTag>
              <Description><![CDATA[Failed to initialize graphics backend for OpenGL.]]></Description>
            </Message>
        """
        logfile = os.path.join(TEMPDIR.name, os.path.basename(mock_test) + ".xml")
        write_xml_log(logfile, failure=None, inject_message=fatal_xml_message)
        del self.env["QT_MOCK_TEST_XML_TEMPLATE_FILE"]
        self.env["QT_TESTRUNNER_DEBUG_NO_UNIQUE_OUTPUT_FILENAME"] = "1"
        self.prepare_env(run_list=["always_pass"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 3)
    def test_qfatal_crash_2(self):
        fatal_xml_message = """
            <Message type="qfatal" file="" line="0">
              <DataTag><![CDATA[modal]]></DataTag>
              <Description><![CDATA[Failed to initialize graphics backend for OpenGL.]]></Description>
            </Message>
        """
        logfile = os.path.join(TEMPDIR.name, os.path.basename(mock_test) + ".xml")
        write_xml_log(logfile, failure="always_fail", inject_message=fatal_xml_message)
        del self.env["QT_MOCK_TEST_XML_TEMPLATE_FILE"]
        self.env["QT_TESTRUNNER_DEBUG_NO_UNIQUE_OUTPUT_FILENAME"] = "1"
        self.prepare_env(run_list=["always_pass,always_fail"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 3)

    def test_qwarn_is_ignored_1(self):
        qwarn_xml_message = """
            <Message type="qwarn" file="" line="0">
              <DataTag><![CDATA[modal]]></DataTag>
              <Description><![CDATA[Failed to create RHI (backend 2)]]></Description>
            </Message>
        """
        logfile = os.path.join(TEMPDIR.name, os.path.basename(mock_test) + ".xml")
        write_xml_log(logfile, failure=None, inject_message=qwarn_xml_message)
        del self.env["QT_MOCK_TEST_XML_TEMPLATE_FILE"]
        self.env["QT_TESTRUNNER_DEBUG_NO_UNIQUE_OUTPUT_FILENAME"] = "1"
        self.prepare_env(run_list=["always_pass"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 0)
    def test_qwarn_is_ignored_2(self):
        fatal_xml_message = """
            <Message type="qfatal" file="" line="0">
              <DataTag><![CDATA[modal]]></DataTag>
              <Description><![CDATA[Failed to initialize graphics backend for OpenGL.]]></Description>
            </Message>
            <Message type="qwarn" file="" line="0">
              <DataTag><![CDATA[modal]]></DataTag>
              <Description><![CDATA[Failed to create RHI (backend 2)]]></Description>
            </Message>
        """
        logfile = os.path.join(TEMPDIR.name, os.path.basename(mock_test) + ".xml")
        write_xml_log(logfile, failure=None, inject_message=fatal_xml_message)
        del self.env["QT_MOCK_TEST_XML_TEMPLATE_FILE"]
        self.env["QT_TESTRUNNER_DEBUG_NO_UNIQUE_OUTPUT_FILENAME"] = "1"
        self.prepare_env(run_list=["always_pass"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 3)

    # If a test returns success but XML contains failures, it's a CRASH.
    def test_wrong_xml_log_written_1_crash(self):
        logfile = os.path.join(TEMPDIR.name, os.path.basename(mock_test) + ".xml")
        write_xml_log(logfile, failure="always_fail")
        del self.env["QT_MOCK_TEST_XML_TEMPLATE_FILE"]
        self.env["QT_TESTRUNNER_DEBUG_NO_UNIQUE_OUTPUT_FILENAME"] = "1"
        self.prepare_env(run_list=["always_pass"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 3)
    # If a test returns failure but XML contains only pass, it's a CRASH.
    def test_wrong_xml_log_written_2_crash(self):
        logfile = os.path.join(TEMPDIR.name, os.path.basename(mock_test) + ".xml")
        write_xml_log(logfile)
        del self.env["QT_MOCK_TEST_XML_TEMPLATE_FILE"]
        self.env["QT_TESTRUNNER_DEBUG_NO_UNIQUE_OUTPUT_FILENAME"] = "1"
        self.prepare_env(run_list=["always_fail"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 3)

    def create_wrapper(self, filename, content=None):
        if not content:
            content='exec "$@"'
        filename = os.path.join(TEMPDIR.name, filename)
        with open(filename, "w") as f:
            f.write(f'#!/bin/sh\n{content}\n')
            self.wrapper_script = f.name
        os.chmod(self.wrapper_script, 0o700)

    # Test that it re-runs the full executable in case of crash, even if the
    # XML file is valid and shows one specific test failing.
    def test_crash_reruns_full_QTQAINFRA_5226(self):
        self.env["QT_MOCK_TEST_RUN_LIST"]      = "always_fail"
        # Tell qt_mock_test to crash after writing a proper XML file.
        self.env["QT_MOCK_TEST_CRASH_CLEANLY"] = "1"
        proc = self.run2()
        # Verify qt-testrunner exited with 3 which means CRASH.
        self.assertEqual(proc.returncode, 3)
        # Verify that a full executable re-run happened that re-runs 2 times,
        # instead of individual functions that re-run 5 times.
        xml_output_files = find_test_logs()
        self.assertEqual(len(xml_output_files), 2)

    # Test that qt-testrunner detects the correct executable name even if we
    # use a special wrapper script, and that it uses that in the XML log filename.
    @unittest.skipUnless(os.name == "posix", "Wrapper script needs POSIX shell")
    def test_wrapper(self):
        self.create_wrapper("coin_vxworks_qemu_runner.sh")
        proc = run_testrunner(wrapper_script=self.wrapper_script,
                              testrunner_args=["--log-dir",TEMPDIR.name],
                              env=self.env)
        self.assertEqual(proc.returncode, 0)
        xml_output_files = find_test_logs()
        self.assertEqual(len(xml_output_files), 1)

    # The "androidtestrunner" wrapper is special. It expects the QTest arguments after "--".
    # So our mock androidtestrunner wrapper ignores everything before "--"
    # and executes our hardcoded mock_test with the arguments that follow.
    def create_mock_anroidtestrunner_wrapper(self):
        self.create_wrapper("androidtestrunner", content=
            'while [ "$1" != "--" ]; do shift; done; shift; exec {} "$@"'.format(mock_test))

    @unittest.skipUnless(os.name == "posix", "Wrapper script needs POSIX shell")
    def test_androidtestrunner_with_aab(self):
        self.create_mock_anroidtestrunner_wrapper()
        # Copied from our CI logs. The only relevant option is --aab.
        androidtestrunner_args= [
            '--path', '/home/qt/work/qt/qtdeclarative_standalone_tests/tests/auto/quickcontrols/qquickpopup/android-build-tst_qquickpopup',
            '--adb', '/opt/android/sdk/platform-tools/adb', '--skip-install-root',
            '--ndk-stack', '/opt/android/android-ndk-r27c/ndk-stack',
            '--manifest', '/home/qt/work/qt/qtdeclarative_standalone_tests/tests/auto/quickcontrols/qquickpopup/android-build-tst_qquickpopup/app/AndroidManifest.xml',
            '--make', '"/opt/cmake-3.30.5/bin/cmake" --build /home/qt/work/qt/qtdeclarative_standalone_tests --target tst_qquickpopup_make_aab',
            '--aab', '/home/qt/work/qt/qtdeclarative_standalone_tests/tests/auto/quickcontrols/qquickpopup/android-build-tst_qquickpopup/tst_qquickpopup.aab',
            '--bundletool', '/opt/bundletool/bundletool', '--timeout', '1425'
        ]
        # In COIN CI, TESTRUNNER="qt-testrunner.py --". That's why we append "--".
        proc = run_testrunner(testrunner_args=["--log-dir", TEMPDIR.name, "--"],
                              wrapper_script=self.wrapper_script,
                              wrapper_args=androidtestrunner_args,
                              env=self.env)
        self.assertEqual(proc.returncode, 0)
        xml_output_files = find_test_logs("tst_qquickpopup")
        self.assertEqual(len(xml_output_files), 1)
    # similar to above but with "--apk"
    @unittest.skipUnless(os.name == "posix", "Wrapper script needs POSIX shell")
    def test_androidtestrunner_with_apk(self):
        self.create_mock_anroidtestrunner_wrapper()
        androidtestrunner_args= ['--blah', '--apk', '/whatever/waza.apk', 'blue']
        proc = run_testrunner(testrunner_args=["--log-dir", TEMPDIR.name, "--"],
                              wrapper_script=self.wrapper_script,
                              wrapper_args=androidtestrunner_args,
                              env=self.env)
        self.assertEqual(proc.returncode, 0)
        xml_output_files = find_test_logs("waza")
        self.assertEqual(len(xml_output_files), 1)
    # similar to above but with neither "--apk" nor "--aab". qt-testrunner throws error.
    @unittest.skipUnless(os.name == "posix", "Wrapper script needs POSIX shell")
    def test_androidtestrunner_fail_to_detect_filename(self):
        self.create_mock_anroidtestrunner_wrapper()
        androidtestrunner_args= ['--blah', '--argh', '/whatever/waza.apk', 'waza.aab']
        proc = run_testrunner(testrunner_args=["--log-dir", TEMPDIR.name, "--"],
                              wrapper_script=self.wrapper_script,
                              wrapper_args=androidtestrunner_args,
                              env=self.env)
        self.assertEqual(proc.returncode, 1)
        xml_output_files = find_test_logs("waza")
        self.assertEqual(len(xml_output_files), 0)


# Test qt-testrunner script with an existing XML log file:
#   qt-testrunner.py qt_mock_test.py --parse-xml-testlog file.xml
# qt-testrunner should repeat the testcases that are logged as
# failures and fail or pass depending on how the testcases behave.
# Different XML files are generated for the following test cases.
#   + No failure logged.                  qt-testrunner should exit(0)
#   + The "always_pass" test has failed.  qt-testrunner should exit(0).
#   + The "always_fail" test has failed.  qt-testrunner should exit(2).
#   + The "always_crash" test has failed. qt-testrunner should exit(3)
#                                         since the re-run will crash.
#   + The "fail_then_pass:2" test failed. qt-testrunner should exit(0).
#   + The "fail_then_pass:5" test failed. qt-testrunner should exit(2).
#   + The "initTestCase" failed which is listed as NO_RERUN thus
#                                         qt-testrunner should exit(3).
class Test_testrunner_with_xml_logfile(unittest.TestCase):
    # Runs before every single test function, creating a unique temp file.
    def setUp(self):
        (_handle, self.xml_file) = mkstemp(
            suffix=".xml", prefix="qt_mock_test-log-",
            dir=TEMPDIR.name)
        os.close(_handle)
        if os.path.exists(os.environ["QT_MOCK_TEST_STATE_FILE"]):
            os.remove(os.environ["QT_MOCK_TEST_STATE_FILE"])
    def tearDown(self):
        os.remove(self.xml_file)
        del self.xml_file

    def test_no_failure(self):
        write_xml_log(self.xml_file, failure=None)
        proc = run_testrunner(self.xml_file)
        self.assertEqual(proc.returncode, 0)
    def test_always_pass_failed(self):
        write_xml_log(self.xml_file, failure="always_pass")
        proc = run_testrunner(self.xml_file)
        self.assertEqual(proc.returncode, 0)
    def test_always_pass_failed_max_repeats_0(self):
        write_xml_log(self.xml_file, failure="always_pass")
        proc = run_testrunner(self.xml_file,
                              testrunner_args=["--max-repeats", "0"])
        self.assertEqual(proc.returncode, 2)
    def test_always_fail_failed(self):
        write_xml_log(self.xml_file, failure="always_fail")
        proc = run_testrunner(self.xml_file)
        self.assertEqual(proc.returncode, 2)
        # Assert that one of the re-runs was in verbose mode
        matches = re.findall("VERBOSE RUN",
                             proc.stdout.decode())
        self.assertEqual(len(matches), 1)
        # Assert that the environment was altered too
        self.assertIn("QT_LOGGING_RULES", proc.stdout.decode())
    def test_always_crash_crashed(self):
        write_xml_log(self.xml_file, failure="always_crash")
        proc = run_testrunner(self.xml_file)
        self.assertEqual(proc.returncode, 3)
    def test_fail_then_pass_2_failed(self):
        write_xml_log(self.xml_file, failure="fail_then_pass:2")
        proc = run_testrunner(self.xml_file)
        self.assertEqual(proc.returncode, 0)
    def test_fail_then_pass_5_failed(self):
        write_xml_log(self.xml_file, failure="fail_then_pass:5")
        proc = run_testrunner(self.xml_file)
        self.assertEqual(proc.returncode, 2)
    def test_with_two_failures(self):
        write_xml_log(self.xml_file,
                      failure=["always_pass", "fail_then_pass:2"])
        proc = run_testrunner(self.xml_file)
        self.assertEqual(proc.returncode, 0)
        # Check that test output is properly interleaved with qt-testrunner's logging.
        matches = re.findall(r"(PASS|FAIL!).*\n.*Test process exited with code",
                             proc.stdout.decode())
        self.assertEqual(len(matches), 4)
    def test_initTestCase_fail_crash(self):
        write_xml_log(self.xml_file, failure="initTestCase")
        proc = run_testrunner(self.xml_file)
        self.assertEqual(proc.returncode, 3)


if __name__ == "__main__":

    DEBUG = False
    if "--debug" in sys.argv:
        sys.argv.remove("--debug")
        DEBUG = True

    # We set failfast=True as we do not want the test suite to continue if the
    # tests of qt_mock_test failed. The next ones depend on it.
    unittest.main(failfast=True)
