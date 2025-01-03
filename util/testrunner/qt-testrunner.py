#!/usr/bin/env python3
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


# !!!IMPORTANT!!!  If you change anything to this script, run the testsuite
#    manually and make sure it still passes, as it doesn't run automatically.
#    Just execute the command line as such:
#
#      ./util/testrunner/tests/tst_testrunner.py -v [--debug]
#
# ======== qt-testrunner ========
#
# This script wraps the execution of a Qt test executable, for example
# tst_whatever, and tries to iron out unpredictable test failures.
# In particular:
#
# + Appends output argument to it: "-o tst_whatever.xml,xml"
# + Checks the exit code. If it is zero, the script exits with zero,
#   otherwise proceeds.
# + Reads the XML test log and Understands exactly which function
#   of the test failed.
#   + If no XML file is found or was invalid, the test executable
#     probably CRASHed, so we *re-run the full test once again*.
# + If some testcases failed it executes only those individually
#   until they pass, or until max-repeats times is reached.
#
# The regular way to use is to set the environment variable TESTRUNNER to
# point to this script before invoking ctest.
#
# NOTE: this script is crafted specifically for use with Qt tests and for
#       using it in Qt's CI. For example it detects and acts specially if test
#       executable is "tst_selftests" or "androidtestrunner".  It also detects
#       env var "COIN_CTEST_RESULTSDIR" and uses it as log-dir.
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
from typing import NamedTuple, Tuple, List, Optional

# Define a custom type for returning a fail incident
class WhatFailed(NamedTuple):
    func: str
    tag: Optional[str] = None


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
    "tst_selftests",                # qtestlib's selftests are using an external test framework (Catch) that does not support -o argument
    "tst_QDoc",                     # Some of QDoc's tests are using an external test framework (Catch) that does not support -o argument
    "tst_QDoc_Catch_Generators",    # Some of QDoc's tests are using an external test framework (Catch) that does not support -o argument
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
    parser.add_argument("--no-extra-args", action="store_true",
                        help="Do not append any extra arguments to the test command line, like"
                        " -o log_file.xml -v2 -vs. This will disable some functionality like the"
                        " failed test repetition and the verbose output on failure. This is"
                        " activated by default when TESTARGS is tst_selftests.")
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

    # QNX test wrapper just needs to be skipped to figure out test_basename
    if args.test_basename == "coin_qnx_qemu_runner.sh":
        args.test_basename = os.path.basename(args.testargs[1])
        L.info("Detected coin_qnx_qemu_runner, test will be handled specially. Detected test basename: %s",
               args.test_basename)

    # On Android emulated platforms, "androidtestrunner" is invoked by CMake
    # to wrap the tests.  We have to append the test arguments to it after
    # "--". Besides that we have to detect the basename to avoid saving the
    # XML log as "androidtestrunner.xml" for all tests.
    if args.test_basename == "androidtestrunner":
        args.specific_extra_args = [ "--" ]
        apk_arg = False
        for a in args.testargs[1:]:
            if a == "--apk":
                apk_arg = True
            elif apk_arg:
                apk_arg = False
                if a.endswith(".apk"):
                    args.test_basename = os.path.basename(a)[:-4]
                    break
        L.info("Detected androidtestrunner, test will be handled specially. Detected test basename: %s",
               args.test_basename)

    if args.test_basename in NON_XML_GENERATING_TESTS:
        L.info("Detected special test not able to generate XML log! Will not parse it and will not repeat individual testcases")
        args.no_extra_args = True
        args.max_repeats = 0

    return args


def parse_log(results_file) -> List[WhatFailed]:
    """Parse the XML test log file. Return the failed testcases, if any.

    Failures are considered the "fail" and "xpass" incidents.
    A testcase is a function with an optional data tag."""
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
        raise AssertionError(
            f"The XML test log must have <TestCase> as root tag, but has: <{root.tag}>")

    failures = []
    n_passes = 0
    for e1 in root:
        if e1.tag == "TestFunction":
            for e2 in e1:                          # every <TestFunction> can have many <Incident>
                if e2.tag == "Incident":
                    if e2.attrib["type"] in ("fail", "xpass"):
                        func = e1.attrib["name"]
                        e3 = e2.find("DataTag")    # every <Incident> might have a <DataTag>
                        if e3 is not None:
                            failures.append(WhatFailed(func, tag=e3.text))
                        else:
                            failures.append(WhatFailed(func))
                    else:
                        n_passes += 1

    end_timer = timeit.default_timer()
    t = end_timer - start_timer
    L.info(f"Parsed XML file {results_file} in {t:.3f} seconds")
    L.info(f"Found {n_passes} passes and {len(failures)} failures")

    return failures


def run_test(arg_list: List[str], **kwargs):
    L.debug("Running test command line: %s", arg_list)
    proc = subprocess.run(arg_list, **kwargs)
    L.info("Test process exited with code: %d", proc.returncode)

    return proc

def unique_filename(test_basename: str) -> str:
    timestamp = round(time.time() * 1000)
    return f"{test_basename}-{timestamp}"

# Returns tuple: (exit_code, xml_logfile)
def run_full_test(test_basename, testargs: List[str], output_dir: str,
                  no_extra_args=False, dryrun=False,
                  timeout=None, specific_extra_args=[])  \
        -> Tuple[int, Optional[str]]:

    results_files = []
    output_testargs = []

    # Append arguments to write log to qtestlib XML file,
    # and text to stdout.
    if not no_extra_args:
        filename_base = unique_filename(test_basename)
        pathname_stem = os.path.join(output_dir, filename_base)
        xml_output_file = f"{pathname_stem}.xml"

        results_files.append(xml_output_file)
        output_testargs.extend([
            "-o", f"{xml_output_file},xml",
            "-o", f"{pathname_stem}.junit.xml,junitxml",
            "-o", f"{pathname_stem}.txt,txt",
            "-o", "-,txt"
            ])

    proc = run_test(testargs + specific_extra_args + output_testargs,
                    timeout=timeout)

    return (proc.returncode, results_files[0] if results_files else None)


def rerun_failed_testcase(test_basename, testargs: List[str], output_dir: str,
                          what_failed: WhatFailed,
                          max_repeats, passes_needed,
                          dryrun=False, timeout=None) -> bool:
    """Run a specific function:tag of a test, until it passes enough times, or
    until max_repeats is reached.

    Return True if it passes eventually, False if it fails.
    """
    assert passes_needed <= max_repeats
    failed_arg = what_failed.func
    if what_failed.tag:
        failed_arg += ":" + what_failed.tag


    n_passes = 0
    for i in range(max_repeats):
        # For the individual testcase re-runs, we log to file since Coin needs
        # to parse it. That is the reason we use unique filename every time.
        filename_base = unique_filename(test_basename)
        pathname_stem = os.path.join(output_dir, filename_base)

        output_args = [
            "-o", f"{pathname_stem}.xml,xml",
            "-o", f"{pathname_stem}.junit.xml,junitxml",
            "-o", f"{pathname_stem}.txt,txt",
            "-o", "-,txt"]
        L.info("Re-running testcase: %s", failed_arg)
        if i < max_repeats - 1:
            proc = run_test(testargs + output_args + [failed_arg],
                            timeout=timeout)
        else:                                                   # last re-run
            proc = run_test(testargs + output_args + VERBOSE_ARGS + [failed_arg],
                            timeout=timeout,
                            env={**os.environ, **VERBOSE_ENV})
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
            failed_functions = []
            if args.parse_xml_testlog:      # do not run test, just parse file
                failed_functions = parse_log(args.parse_xml_testlog)
                # Pretend the test returned correct exit code
                retcode = len(failed_functions)
            else:                                # normal invocation, run test
                (retcode, results_file) = \
                    run_full_test(args.test_basename, args.testargs, args.log_dir,
                                  args.no_extra_args, args.dry_run, args.timeout,
                                  args.specific_extra_args)
                if results_file:
                    failed_functions = parse_log(results_file)

            if retcode == 0:
                if failed_functions:
                    L.warning("The test executable returned success but the logfile"
                             f" contains FAIL for function: {failed_functions[0].func}")
                    continue
                sys.exit(0)    # PASS

            if len(failed_functions) == 0:
                L.warning("No failures listed in the XML test log!"
                          " Did the test CRASH right after all its testcases PASSed?")
                continue

            cant_rerun = [ f.func for f in failed_functions if f.func in NO_RERUN_FUNCTIONS ]
            if cant_rerun:
                L.warning(f"Failure detected in the special test function '{cant_rerun[0]}'"
                          " which can not be re-run individually")
                continue

            assert len(failed_functions) > 0  and  retcode != 0
            break    # all is fine, goto re-running individual failed testcases

        except Exception as e:
            L.error("exception:%s %s", type(e).__name__, e)
            L.error("The test executable probably crashed, see above for details")

    if args.max_repeats == 0:
        sys.exit(2)          # Some tests failed but no re-runs were asked

    L.info("Some tests failed, will re-run at most %d times.\n",
           args.max_repeats)

    for what_failed in failed_functions:
        try:
            ret = rerun_failed_testcase(args.test_basename, args.testargs, args.log_dir,
                                        what_failed, args.max_repeats, args.passes_needed,
                                        dryrun=args.dry_run, timeout=args.timeout)
        except Exception as e:
            L.error("exception:%s %s", type(e).__name__, e)
            L.error("The testcase re-run probably crashed, giving up")
            sys.exit(3)                                    # Test re-run CRASH

        if not ret:
            sys.exit(2)                                    # Test re-run FAIL

    sys.exit(0)                                  # All testcase re-runs PASSed


if __name__ == "__main__":
    main()
