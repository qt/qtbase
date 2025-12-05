#!/usr/bin/env python3
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


# !!!IMPORTANT!!!  If you change anything to this script, run the testsuite
#   and make sure it still passes:
#
#       qtbase/tests/auto/util/testrunner/tst_qt_testrunner.py -v [--debug]
#
# ======== qt-testrunner ========
#
# This script wraps the execution of a Qt test executable, for example
# tst_whatever, and tries to iron out unpredictable test failures.
# It also reruns a new or modified test function multiple times
# to ensure its stability.
# In particular:
#
# + Append output argument to it: "-o tst_whatever.xml,xml" and
#   execute it.
# + Save the exit code.
#   - If it is <0 or >=128 (see NOTE_2), mark the test run as CRASH.
# + Read the XML test log and find exactly which functions
#   of the test FAILed.
# + Mark the test run as CRASH, if:
#   - no XML file is found,
#   - or an invalid XML file is found,
#   - or the XML contains a QFatal message: <Message type="qfatal">
#   - or no test FAILures are listed in the XML but the saved
#     exit code is not 0.
# + If, based on the rules above, the test run is marked as CRASH,
#   then *re-run the full test once again* and start this logic over.
#   If we are on the 2nd run and CRASH happens again, then exit(3).
# + Examine the saved exit code:
#   if it is 0, then exit(0) (success, all tests have PASSed).
# + Otherwise, some testcases failed, so execute only those individually
#   until they pass, or until max-repeats (default: 5) times is reached.
#
# The regular way to use is to set the environment variable TESTRUNNER to
# point to this script before invoking ctest. In COIN CI it is set as
# TESTRUNNER="qt-testrunner.py --" to stop it from parsing further args.
#
# NOTE: this script is crafted specifically for use with Qt tests and for
#       using it in Qt's CI. For example it detects and acts specially if test
#       executable is "tst_selftests" or "androidtestrunner".  It also detects
#       env var "COIN_CTEST_RESULTSDIR" and uses it as log-dir.
#
# NOTE_2: Why is qt-testrunner considering exit code outside [0,127] as CRASH?
#         On Linux, Python subprocess module returns positive `returncode`
#         (255 for example), even if the child does exit(-1 for example). It
#         returns negative `returncode` only if the child is killed by a signal.
#         Qt-testrunner wants to catch both of these cases as CRASH.
#         On Windows, a crash is usually accompanied by exitcode >= 0xC0000000.
#         Finally, QTest is limiting itself to exit codes in [0,127]
#         so anything outside that range is abnormal, thus treated as CRASH.
#
# TODO implement --dry-run.

# Exit codes of this script:
#   0: PASS. Either no test failed, or failed initially but passed
#      in the re-runs (FLAKY PASS).
#   1: Some unexpected error of this script.
#   2: FAIL! for at least one test, even after the individual re-runs.
#   3: CRASH! for the test executable even after re-running it once.
#        Or when we can't re-run individual functions for any reason.


import sys
if sys.version_info < (3, 6):
    sys.stderr.write(
        "Error: this test wrapper script requires Python version 3.6 at least\n")
    sys.exit(1)

import argparse
import subprocess
import os
import traceback
import time
import timeit
import xml.etree.ElementTree as ET
import logging as L

from pprint import pprint
from typing import NamedTuple, Tuple, List, Set, Optional

# Define a custom type for returning a fail incident
class TestResult(NamedTuple):
    func: str
    tag: Optional[str] = None
class WhatFailed(NamedTuple):
    qfatal_message: Optional[str]    = None
    failed_tests:   List[TestResult] = []

class ReRunCrash(Exception):
    pass
class BadXMLCrash(Exception):
    pass


# In the last test re-run, we add special verbosity arguments, in an attempt
# to log more information about the failure
VERBOSE_ARGS = ["-v2", "-maxwarnings", "0"]
VERBOSE_ENV = {
    "QT_LOGGING_RULES": "*=true",
    "QT_MESSAGE_PATTERN": "[%{time process} %{if-debug}D%{endif}%{if-warning}W%{endif}%{if-critical}C%{endif}%{if-fatal}F%{endif}] %{category} %{file}:%{line} %{function}()  -  %{message}",
}
# The following special function names can not re-run individually.
NO_RERUN_FUNCTIONS = {
    "initTestCase", "init", "cleanup", "cleanupTestCase"
}
# The following tests do not write XML log files properly. qt-testrunner will
# not try to append "-o" to their command-line or re-run failed testcases.
# Only add tests here if absolutely necessary!
NON_XML_GENERATING_TESTS = {
    # These tests use an external test framework (Catch) that doesn't support
    # QtTest's -o argument.
    "tst_selftests",
    "tst_QDoc",
    "tst_QDoc_Catch_Generators",
    "tst_androidtestrunner.py",
}
# These are scripts that are used to wrap test execution for special platforms.
# They need special handling (most times just skipping the wrapper name in argv[]).
TEST_RUNNER_WRAPPERS = {
    "coin_qnx_qemu_runner.sh",
    "coin_vxworks_qemu_runner.sh",
    "androidtestrunner",                # extra special handling needed, see code below.
}


def parse_args():
    parser = argparse.ArgumentParser(formatter_class=argparse.RawDescriptionHelpFormatter,
        description="""
Wrap Qt test execution. This is intended to be invoked via the TESTRUNNER
environment variable before running ctest in the CI environment. The purpose
of the script is to repeat failed tests in order to iron out transient errors
caused by unpredictable factors. Individual test functions that failed are
retried up to max-repeats times until the test passes.
        """,
        epilog="""
Default flags: --max-repeats 5 --passes-needed 1
        """
    )
    parser.add_argument("testargs", metavar="TESTARGS", nargs="+",
                        help="Test executable and arguments")
    parser.add_argument("--log-dir", metavar="DIR",
                        help="Where to write the XML log files with the test results of the primary test run;"
                        " by default write to CWD")
    parser.add_argument("--max-repeats", type=int, default=5, metavar='N',
                        help="In case the test FAILs, repeat the failed cases this many times")
    parser.add_argument("--passes-needed", type=int, default=1, metavar='M',
                        help="Number of repeats that need to succeed in order to return an overall PASS")
    parser.add_argument("--parse-xml-testlog", metavar="file.xml",
                        help="Do not run the full test the first time, but parse this XML test log;"
                        " if the test log contains failures, then re-run the failed cases normally,"
                        " as indicated by the other flags")
    parser.add_argument("--dry-run", action="store_true",
                        help="(TODO - not implemented yet) Do not run anything, just describe what would happen")
    parser.add_argument("--timeout", metavar="T",
                        help="Timeout for each test execution in seconds")
    parser.add_argument("--modified-functions-file", dest="mod_file",
                        help="File with modified test functions")
    parser.add_argument("--repeat-modified", type=int, dest="repeat_modified", default=10,
                        help="Number of repeats that a new / modified test function has to succeed in order to return a PASS")
    parser.add_argument("--no-extra-args", action="store_true",
                        help="Do not append any extra arguments to the test command line, like"
                        " -o log_file.xml -v2 -vs. This will disable some functionality like the"
                        " failed test repetition and the verbose output on failure. This is"
                        " activated by default when TESTARGS is tst_selftests.")
    # TODO parser.parse_args(args=sys.argv[0:cmd_index]).
    #   Where cmd_index is either the first positional argument, or the argument right after "--".
    #   This way it won't interpet arguments after the first positional arg.
    args = parser.parse_args()
    args.self_name = os.path.basename(sys.argv[0])
    args.specific_extra_args = []

    logging_format = args.self_name + " %(levelname)8s: %(message)s"
    L.basicConfig(format=logging_format, level=L.DEBUG)

    if args.log_dir is None:
        if "COIN_CTEST_RESULTSDIR" in os.environ:
            args.log_dir = os.environ["COIN_CTEST_RESULTSDIR"]
            L.info("Will write XML test logs to directory"
                   " COIN_CTEST_RESULTSDIR=%s", args.log_dir)
        else:
            args.log_dir = "."

    args.test_basename = os.path.basename(args.testargs[0])

    if args.test_basename.endswith(".exe"):
        args.test_basename = args.test_basename[:-4]

    # When tests are cross-compiled. such as on WASM or iOS, CMake runs them
    # through a CROSSCOMPILING_EMULATOR, prepended to the real test executable.
    emulator = os.environ.get("CMAKE_CROSSCOMPILING_EMULATOR", "").split(";")
    if emulator[0] and args.test_basename == os.path.basename(emulator[0]):
        args.test_basename = os.path.basename(args.testargs[len(emulator)])

    # On Android emulated platforms, "androidtestrunner" is invoked by CMake
    # to wrap the tests.  We have to invoke it with all its arguments, and
    # then append "--" and append our QTest-specific arguments.
    #
    # Besides that we have to detect the basename to avoid saving the XML log
    # as "androidtestrunner.xml" for all tests. To do that we look for the
    # "--apk" or "--aab" option and read its argument.
    elif args.test_basename == "androidtestrunner":
        if "--" in args.testargs:
            L.critical("qt-testrunner can't handle pre-existing '--' argument to androidtestrunner")
            sys.exit(1)
        args.specific_extra_args = [ "--" ]
        apk_arg = False
        aab_arg = False
        for a in args.testargs[1:]:
            if a == "--apk":
                apk_arg = True
            elif apk_arg:
                apk_arg = False
                if a.endswith(".apk"):
                    args.test_basename = os.path.basename(a)[:-4]
                    break
            elif a == "--aab":
                aab_arg = True
            elif aab_arg:
                aab_arg = False
                if a.endswith(".aab"):
                    args.test_basename = os.path.basename(a)[:-4]
                    break
        L.info("Detected androidtestrunner, test will be handled specially. Detected test basename: %s",
               args.test_basename)
        if args.test_basename == "androidtestrunner":
            L.critical("Couldn't detect the test's basename")
            sys.exit(1)

    # Test wrapper just needs to be skipped to figure out test_basename.
    elif args.test_basename in TEST_RUNNER_WRAPPERS:
        args.test_basename = os.path.basename(args.testargs[1])

    if args.test_basename in NON_XML_GENERATING_TESTS:
        L.info("Detected special test not able to generate XML log! Will not parse it and will not repeat individual testcases")
        args.no_extra_args = True
        args.max_repeats = 0

    args.modified_functions = set()
    if args.mod_file:
        args.modified_functions = read_modified_functions_file(args.mod_file, args.test_basename)
    else:
        if "COIN_CTEST_RESULTSDIR" in os.environ:
            # CI default: modified functions file is installed together with qt-testrunner.py.
            mydir = os.path.dirname(os.path.abspath(__file__))
            filename = os.path.join(mydir, "qt-modified-testfunctions.txt")
            try:
                args.modified_functions = read_modified_functions_file(filename, args.test_basename)
            except OSError as e:
                L.warning("Skipping reading %s (%s)",
                          filename, e.strerror)

    if args.modified_functions:
        L.info("Modified functions found, they have to pass %d times:\n    %s",
               args.repeat_modified, " ".join(args.modified_functions))

    return args


def parse_log(results_file) -> WhatFailed:
    """
    Parse the XML test log file. Return the failed testcases, if any,
    and the first qfatal message possibly printed.

    Failures are considered the "fail" and "xpass" incidents.
    """
    start_timer = timeit.default_timer()

    try:
        tree = ET.parse(results_file)
    except FileNotFoundError:
        L.error("XML log file not found: %s", results_file)
        raise
    except Exception as e:
        L.error("Failed to parse the XML log file: %s", results_file)
        with open(results_file, "rb") as f:
            if os.stat(f.fileno()).st_size == 0:
                L.error("    File is empty")
            else:
                L.error("    File Contents:\n%s\n\n",
                        f.read().decode("utf-8", "ignore"))
        raise

    root = tree.getroot()
    if root.tag != "TestCase":
        raise BadXMLCrash(
            f"The XML test log must have <TestCase> as root tag, but has: <{root.tag}>")

    failures = []
    qfatal_message = None

    n_passes = 0
    for e1 in root:
        if e1.tag == "TestFunction":
            for e2 in e1:                          # every <TestFunction> can have many <Incident>
                if e2.tag == "Incident":
                    if e2.attrib["type"] in ("fail", "xpass"):
                        func = e1.attrib["name"]
                        datatag = None
                        e3 = e2.find("DataTag")    # every <Incident> might have a <DataTag>
                        if e3 is not None:
                            datatag = e3.text
                        failures.append(TestResult(func, datatag))
                    else:
                        n_passes += 1

    # Use iter() here to _recursively_ search root for <Message>,
    # as we don't trust that messages are always at the same depth.
    for message_tag in root.iter(tag="Message"):
        messagetype = message_tag.get("type")
        if messagetype == "qfatal":
            message_desc = message_tag.find("Description")
            if message_desc is not None:
                qfatal_message = message_desc.text
            else:
                qfatal_message = "--EMPTY QFATAL--"
            L.warning("qFatal message ('%s') found in the XML, treating this run as a CRASH!",
                      qfatal_message)
            break

    end_timer = timeit.default_timer()
    t = end_timer - start_timer
    L.info(f"Parsed XML file {results_file} in {t:.3f} seconds")
    L.info(f"Found {n_passes} passes and {len(failures)} failures")

    return WhatFailed(qfatal_message, failures)


def run_test(arg_list: List[str], **kwargs):
    if (os.environ.get("QT_TESTRUNNER_TESTING", "0") == "1"
        and os.name == "nt"
        and arg_list[0].endswith("qt_mock_test")
    ):
        # For executing qt_mock_test under the same Python interpreter when testing.
        arg_list = [ sys.executable ] + arg_list
    L.debug("Running test command line: %s", arg_list)
    proc = subprocess.run(arg_list, **kwargs)
    L.info("Test process exited with code: %d", proc.returncode)

    return proc

def unique_filename(test_basename: str) -> str:

    # Hidden env var for testing, enforcing a predictable, non-unique filename.
    if os.environ.get("QT_TESTRUNNER_DEBUG_NO_UNIQUE_OUTPUT_FILENAME"):
        return f"{test_basename}"

    timestamp = round(time.time() * 1000)
    return f"{test_basename}-{timestamp}"

def generate_qtest_output_args(pathname_stem : str) -> List[str]:
    out_args = [
        "-o", f"{pathname_stem}.xml,xml",
        "-o", f"{pathname_stem}.junit.xml,junitxml",
        "-o", f"{pathname_stem}.txt,txt",
        "-o", "-,txt"]
    return out_args


# Returns tuple: (exit_code, xml_logfile)
def run_full_test(test_basename, testargs: List[str], output_dir: str,
                  no_extra_args=False, dryrun=False,
                  timeout=None, specific_extra_args=[])  \
        -> Tuple[int, Optional[str]]:

    xml_output_file = None
    output_testargs = []

    # Append arguments to write log to qtestlib XML file,
    # and text to stdout.
    if not no_extra_args:
        filename_base = unique_filename(test_basename)
        pathname_stem = os.path.join(output_dir, filename_base)
        output_testargs = generate_qtest_output_args(pathname_stem)
        xml_output_file = f"{pathname_stem}.xml"

    proc = run_test(testargs + specific_extra_args + output_testargs,
                    timeout=timeout)

    return (proc.returncode, xml_output_file)


def rerun_failed_testcase(test_basename, testargs: List[str], output_dir: str,
                          testcase: TestResult,
                          max_repeats, passes_needed,
                          dryrun=False, timeout=None) -> bool:
    """Run a specific function:tag of a test, until it passes enough times, or
    until max_repeats is reached.

    Return True if it passes eventually, False if it fails.
    Raise ReRunCrash Exception if it crashes.
    """
    assert passes_needed <= max_repeats
    failed_arg = testcase.func
    if testcase.tag:
        failed_arg += ":" + testcase.tag

    n_passes = 0
    for i in range(max_repeats):
        # For the individual testcase re-runs, we log to file since Coin needs
        # to parse it. That is the reason we use unique filename every time.
        filename_base = unique_filename(test_basename)
        pathname_stem = os.path.join(output_dir, filename_base)
        output_args = generate_qtest_output_args(pathname_stem)

        L.info("Re-running testcase: %s", failed_arg)
        if i < max_repeats - 1:
            proc = run_test(testargs + output_args + [failed_arg],
                            timeout=timeout)
        else:                                                   # last re-run
            proc = run_test(testargs + output_args + VERBOSE_ARGS + [failed_arg],
                            timeout=timeout,
                            env={**os.environ, **VERBOSE_ENV})
        # There are platforms that run tests wrapped with some test-runner
        # script, that can possibly fail to extract a process exit code.
        # Because of these cases, we *also* parse the XML file and signify
        # CRASH in case of QFATAL/empty/corrupt result.
        what_failed = parse_log(f"{pathname_stem}.xml")
        if what_failed.qfatal_message:
            raise ReRunCrash(f"CRASH! returncode:{proc.returncode} "
                             f"QFATAL:'{what_failed.qfatal_message}'")
        if proc.returncode < 0 or proc.returncode >= 128:
            raise ReRunCrash(f"CRASH! returncode:{proc.returncode}")
        if proc.returncode == 0 and len(what_failed.failed_tests) > 0:
            raise ReRunCrash("CRASH! returncode:0 but failures were found: "
                             + str(what_failed.failed_tests))
        if proc.returncode == 0:
            n_passes += 1
        if n_passes == passes_needed:
            L.info("Test has PASSed as FLAKY after re-runs:%d, passes:%d, failures:%d",
                   i+1, n_passes, i+1-n_passes)
            return True

    assert n_passes <  passes_needed
    assert n_passes <= max_repeats
    n_failures = max_repeats - n_passes
    L.info("Test has FAILed despite all repetitions! re-runs:%d failures:%d",
           max_repeats, n_failures)
    return False

# Expecting a text file where each line is formatted as follows:
# void tst_testClass::testFunction()
# void tst_testClass::testFunction() const
#
# TODO: Inherited test functions are not handled. When a derived test class
#   (e.g.  tst_QGuiApplication : public tst_QCoreApplication) inherits test
#   methods from a base class, edits to the base class's .cpp are recorded
#   under the base class name and will not match test_basename of the derived
#   class here. To fix, the caller would need to supply not just the test's
#   own basename but also the set of base-class basenames whose modified
#   functions should be treated as belonging to this test, and this function
#   would need to accept matches against any of those names.
def read_modified_functions_file(filename: str, test_basename: str) -> Set[str]:
    modified_functions : Set[str] = set()
    if not os.path.isfile(filename):
        L.info("Modified-functions file '%s' doesn't exist."
               " Assuming no modified / new test functions to be handled.",
               filename)
        return modified_functions
    with open(filename, "r") as file:
        for line in file:
            line = line.strip()
            if not line or "::" not in line:
                L.warning("While parsing modified functions from %s, skipping malformed line: %s",
                          filename, line)
                continue
            try:
                parts = line.split("::")
                test_class = parts[0].removeprefix("void").strip()
                test_function = parts[-1].removesuffix(" const").rstrip()
                test_function = test_function.removesuffix("()").strip()
                test_function = test_function.removesuffix("_data")
                if test_class.lower() == test_basename.lower():
                    modified_functions.add(test_function)
            except (IndexError, ValueError):
                L.warning("While parsing modified functions file, skipping malformed line: %s",
                          line)

    return modified_functions

def rerun_modified_test_functions(args: argparse.Namespace) -> int:
    if not args.modified_functions:
        return 0
    if args.no_extra_args:
        L.info(f"Test executable '%s' contains modified-functions (%s) but it is not built on QTest."
               " Can't rerun any specific function, finishing.",
               args.test_basename, str(args.modified_functions))
        return 0
    ret = 0
    for modified_function in args.modified_functions:
        filename_base = unique_filename(args.test_basename)
        pathname_stem = os.path.join(args.log_dir, filename_base)
        output_args = generate_qtest_output_args(pathname_stem)
        not_passes = 0
        # We already had iteration 1, so here come 2 through repeat_modified:
        for iteration in range(2, args.repeat_modified + 1):
            L.info("Re-running modified test function %s (%d/%d times)",
                    modified_function, iteration, args.repeat_modified)
            proc = run_test(args.testargs + output_args + [modified_function],
                            timeout=args.timeout)
            if proc.returncode < 0 or proc.returncode >= 128:   # CRASH
                # We don't raise ReRunCrash Exception here because the program
                # is exiting right after anyway, and we don't want to affect this.
                ret = 3
                not_passes += 1
            elif proc.returncode != 0:                          # FAIL
                if ret != 3:       # preserve any previous crash exit code (3)
                    ret = 2
                not_passes += 1
        if not_passes > 0:
            L.warning("Modified test function '%s' failed %d/%d times!",
                      modified_function, not_passes, args.repeat_modified)

    return ret


def main():
    args = parse_args()
    n_full_runs = 1 if args.parse_xml_testlog else 2

    for i in range(n_full_runs + 1):

        if 0 < i < n_full_runs:
            L.info("Will re-run the full test executable")
        elif i == n_full_runs:    # Failed on the final run
            L.error("Full test run failed repeatedly, aborting!")
            sys.exit(3)

        try:
            results_file = None
            what_failed = WhatFailed()
            if args.parse_xml_testlog:      # do not run test, just parse file
                what_failed = parse_log(args.parse_xml_testlog)
                # Pretend the test returned correct exit code
                retcode = len(what_failed.failed_tests)
            else:                                # normal invocation, run test
                (retcode, results_file) = \
                    run_full_test(args.test_basename, args.testargs, args.log_dir,
                                  args.no_extra_args, args.dry_run, args.timeout,
                                  args.specific_extra_args)
                if results_file:
                    what_failed = parse_log(results_file)

            failed_functions = what_failed.failed_tests
            if failed_functions and args.modified_functions:
                failed_modified_functions = [tr.func for tr in failed_functions
                                                     if tr.func in args.modified_functions]
                if failed_modified_functions:
                    L.info("New or modified functions failed early: %s",
                           " ".join(failed_modified_functions))
                    L.info("Exiting without re-running anything!")
                    sys.exit(2)
            if failed_functions and retcode == 0:
                L.warning("The test executable returned success but the logfile"
                         f" contains FAIL for function: {failed_functions[0].func}")
                continue    # CRASH

            if retcode < 0 or retcode >= 128 or what_failed.qfatal_message:
                L.warning("CRASH detected, re-running the whole executable")
                continue    # CRASH

            ret_modified = rerun_modified_test_functions(args)
            if ret_modified != 0:
                sys.exit(ret_modified)

            # All (if any) modified-function reruns have PASSed!
            if retcode == 0:
                sys.exit(0)

            if len(failed_functions) == 0:
                if results_file:
                    L.warning("No failures listed in the XML test log!"
                              " Did the test CRASH right after all its testcases PASSed?")
                continue    # CRASH

            cant_rerun = [ f.func for f in failed_functions if f.func in NO_RERUN_FUNCTIONS ]
            if cant_rerun:
                L.warning(f"Failure detected in the special test function '{cant_rerun[0]}'"
                          " which can not be re-run individually")
                continue    # CRASH

            assert len(failed_functions) > 0  and  retcode != 0
            break    # all is fine, goto re-running individual failed testcases

        except AssertionError:
            raise
        except Exception as e:
            L.error("exception:%s %s", type(e).__name__, e)
            L.error("The test executable probably crashed, see above for details")

    if args.max_repeats == 0:
        sys.exit(2)          # Some tests failed but no re-runs were asked

    L.info("%d test(s) failed, will re-run at most %d times.\n",
           len(failed_functions), args.max_repeats)

    for test_result in failed_functions:
        # Re-run a failed test function max_repeats times, in order to iron-out flakiness.
        try:
            ret = rerun_failed_testcase(args.test_basename, args.testargs, args.log_dir,
                                        test_result, args.max_repeats, args.passes_needed,
                                        dryrun=args.dry_run, timeout=args.timeout)
        except AssertionError:
            raise
        except Exception as e:
            L.error("exception:%s", e)
            L.error("The testcase re-run probably crashed, giving up")
            sys.exit(3)                                    # Test re-run CRASH

        if not ret:
            sys.exit(2)                                    # Test re-run FAIL

    sys.exit(0)                                  # All testcase re-runs PASSed


if __name__ == "__main__":
    main()
