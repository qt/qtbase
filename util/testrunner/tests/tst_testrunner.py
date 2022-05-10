#!/usr/bin/env python3
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


import sys
import os
import re
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
def run(*args, **kwargs):
    if DEBUG:
        print("Running: ", args, flush=True)
    proc = subprocess.run(*args, stdout=PIPE, stderr=STDOUT, **kwargs)
    if DEBUG and proc.stdout:
        print(proc.stdout.decode(), flush=True)
    return proc

# Helper to run qt-testrunner.py with proper testing arguments.
def run_testrunner(xml_filename=None, extra_args=None, env=None):

    args = [ testrunner, mock_test ]
    if xml_filename:
        args += [ "--parse-xml-testlog", xml_filename ]
    if extra_args:
        args += extra_args

    return run(args, env=env)

# Write the XML_TEMPLATE to filename, replacing the templated results.
def write_xml_log(filename, failure=None):
    data = XML_TEMPLATE
    # Replace what was asked to fail with "fail"
    if type(failure) in (list, tuple):
        for template in failure:
            data = data.replace("{{"+template+"_result}}", "fail")
    elif type(failure) is str:
        data = data.replace("{{"+failure+"_result}}", "fail")
    # Replace the rest with "pass"
    data = re.sub(r"{{[^}]+}}", "pass", data)
    with open(filename, "w") as f:
        f.write(data)


# Test that qt_mock_test.py behaves well. This is necessary to properly
# test qt-testrunner.
class Test_qt_mock_test(unittest.TestCase):
    def setUp(self):
        state_file = os.environ["QT_MOCK_TEST_STATE_FILE"]
        if os.path.exists(state_file):
            os.remove(state_file)
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
    def test_xml_file_is_written(self):
        filename = os.path.join(TEMPDIR.name, "testlog.xml")
        proc = run([mock_test, "-o", filename+",xml"])
        self.assertEqual(proc.returncode, 0)
        self.assertTrue(os.path.exists(filename))
        self.assertGreater(os.path.getsize(filename), 0)
        os.remove(filename)

# Test regular invocations of qt-testrunner.
class Test_testrunner(unittest.TestCase):
    def setUp(self):
        state_file = os.environ["QT_MOCK_TEST_STATE_FILE"]
        if os.path.exists(state_file):
            os.remove(state_file)
        old_logfile = os.path.join(TEMPDIR.name, os.path.basename(mock_test) + ".xml")
        if os.path.exists(old_logfile):
            os.remove(old_logfile)
        self.env = dict()
        self.env["QT_MOCK_TEST_XML_TEMPLATE_FILE"] = os.environ["QT_MOCK_TEST_XML_TEMPLATE_FILE"]
        self.env["QT_MOCK_TEST_STATE_FILE"]        = state_file
        self.extra_args = [ "--log-dir", TEMPDIR.name ]
    def prepare_env(self, run_list=None):
        if run_list is not None:
            self.env['QT_MOCK_TEST_RUN_LIST'] = ",".join(run_list)
    def run2(self):
        return run_testrunner(extra_args=self.extra_args, env=self.env)
    def test_simple_invocation(self):
        # All tests pass.
        proc = self.run2()
        self.assertEqual(proc.returncode, 0)
    def test_always_pass(self):
        self.prepare_env(run_list=["always_pass"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 0)
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

    # If no XML file is found by qt-testrunner, it is usually considered a
    # CRASH and the whole test is re-run. Even when the return code is zero.
    # It is a PASS only if the test is not capable of XML output (see no_extra_args, TODO test it).
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

    # If a test returns success but XML contains failures, it's a CRASH.
    def test_wrong_xml_log_written_1_crash(self):
        logfile = os.path.join(TEMPDIR.name, os.path.basename(mock_test) + ".xml")
        write_xml_log(logfile, failure="always_fail")
        del self.env["QT_MOCK_TEST_XML_TEMPLATE_FILE"]
        self.prepare_env(run_list=["always_pass"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 3)
    # If a test returns failure but XML contains only pass, it's a CRASH.
    def test_wrong_xml_log_written_2_crash(self):
        logfile = os.path.join(TEMPDIR.name, os.path.basename(mock_test) + ".xml")
        write_xml_log(logfile)
        del self.env["QT_MOCK_TEST_XML_TEMPLATE_FILE"]
        self.prepare_env(run_list=["always_fail"])
        proc = self.run2()
        self.assertEqual(proc.returncode, 3)


# Test qt-testrunner script with an existing XML log file:
#   qt-testrunner.py qt_mock_test.py --parse-xml-testlog file.xml
# qt-testrunner should repeat the testcases that are logged as
# failures and fail or pass depending on how the testcases behave.
# Different XML files are generated for the following test cases.
#   + No failure logged.                  qt-testrunner should exit(0)
#   + The "always_pass" test has failed.  qt-testrunner should exit(0).
#   + The "always_fail" test has failed.  qt-testrunner should exit(2).
#   + The "always_crash" test has failed. qt-testrunner should exit(2).
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
                              extra_args=["--max-repeats", "0"])
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
    def test_always_crash_failed(self):
        write_xml_log(self.xml_file, failure="always_crash")
        proc = run_testrunner(self.xml_file)
        self.assertEqual(proc.returncode, 2)
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
