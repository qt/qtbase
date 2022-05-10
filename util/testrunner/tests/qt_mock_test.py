#!/usr/bin/env python3
# Copyright (C) 2021 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


# This is an artificial test, mimicking the Qt tests, for example tst_whatever.
# Its purpose is to assist in testing qt-testrunner.py.
#
# Mode A:
#
# If invoked with a test function argument, it runs that test function.
#
# Usage:
#
# $0 always_pass
# $0 always_fail
# $0 always_crash
# $0 fail_then_pass:N   # where N is the number of failing runs before passing
#
# Needs environment variable:
#   + QT_MOCK_TEST_STATE_FILE :: points to a unique filename, to be written
#     for keeping the state of the fail_then_pass:N tests.
#
# Mode B:
#
# If invoked without any argument, it runs the tests listed in the
# variable QT_MOCK_TEST_FAIL_LIST. If variable is empty it just runs
# the always_pass test. It also understands qtestlib's `-o outfile.xml,xml`
# option for writing a mock testlog in a file. Requires environment variables:
#   + QT_MOCK_TEST_STATE_FILE :: See above
#   + QT_MOCK_TEST_XML_TEMPLATE_FILE :: may point to the template XML file
#     located in the same source directory. Without this variable, the
#     option `-o outfile.xml,xml` will be ignored.
#   + QT_MOCK_TEST_FAIL_LIST :: may contain a comma-separated list of test
#     that should run.


import sys
import os
import traceback
from tst_testrunner import write_xml_log


MY_NAME         = os.path.basename(sys.argv[0])
STATE_FILE      = None
XML_TEMPLATE    = None
XML_OUTPUT_FILE = None


def put_failure(test_name):
    with open(STATE_FILE, "a") as f:
        f.write(test_name + "\n")
def get_failures(test_name):
    n = 0
    try:
        with open(STATE_FILE) as f:
            for line in f:
                if line.strip() == test_name:
                    n += 1
    except FileNotFoundError:
        return 0
    return n

# Only care about the XML log output file.
def parse_output_argument(a):
    global XML_OUTPUT_FILE
    if a.endswith(",xml"):
        XML_OUTPUT_FILE = a[:-4]

# Strip qtestlib specific arguments.
# Only care about the "-o ...,xml" argument.
def clean_cmdline():
    args = []
    prev_arg = None
    skip_next_arg = True    # Skip argv[0]
    for a in sys.argv:
        if skip_next_arg:
            if prev_arg == "-o":
                parse_output_argument(a)
            prev_arg = None
            skip_next_arg = False
            continue
        if a in ("-o", "-maxwarnings"):
            skip_next_arg = True
            prev_arg = a
            continue
        if a in ("-v1", "-v2", "-vs"):
            print("VERBOSE RUN")
            if "QT_LOGGING_RULES" in os.environ:
                print("Environment has QT_LOGGING_RULES:",
                      os.environ["QT_LOGGING_RULES"])
            continue
        args.append(a)
    return args


def log_test(testcase, result,
             testsuite=MY_NAME.rpartition(".")[0]):
    print("%-7s: %s::%s()" % (result, testsuite, testcase))

# Return the exit code
def run_test(testname):
    if   testname == "initTestCase":
        exit_code = 1              # specifically test that initTestCase fails
    elif testname == "always_pass":
        exit_code = 0
    elif testname == "always_fail":
        exit_code = 1
    elif testname == "always_crash":
        exit_code = 130
    elif testname.startswith("fail_then_pass"):
        wanted_fails   = int(testname.partition(":")[2])
        previous_fails = get_failures(testname)
        if previous_fails < wanted_fails:
            put_failure(testname)
            exit_code = 1
        else:
            exit_code = 0
    else:
        assert False, "Unknown argument: %s" % testname

    if exit_code == 0:
        log_test(testname, "PASS")
    elif exit_code == 1:
        log_test(testname, "FAIL!")
    else:
        log_test(testname, "CRASH!")

    return exit_code

def no_args_run():
    try:
        run_list = os.environ["QT_MOCK_TEST_RUN_LIST"].split(",")
    except KeyError:
        run_list = ["always_pass"]

    total_result = True
    fail_list = []
    for test in run_list:
        test_exit_code = run_test(test)
        if test_exit_code not in (0, 1):
            sys.exit(130)                                       # CRASH!
        if test_exit_code != 0:
            fail_list.append(test)
        total_result = total_result and (test_exit_code == 0)

    if XML_TEMPLATE and XML_OUTPUT_FILE:
        write_xml_log(XML_OUTPUT_FILE, failure=fail_list)

    if total_result:
        sys.exit(0)
    else:
        sys.exit(1)


def main():
    global STATE_FILE
    # Will fail if env var is not set.
    STATE_FILE = os.environ["QT_MOCK_TEST_STATE_FILE"]

    global XML_TEMPLATE
    if "QT_MOCK_TEST_XML_TEMPLATE_FILE" in os.environ:
        with open(os.environ["QT_MOCK_TEST_XML_TEMPLATE_FILE"]) as f:
            XML_TEMPLATE = f.read()

    args = clean_cmdline()

    if len(args) == 0:
        no_args_run()
        assert False, "Unreachable!"
    else:
        sys.exit(run_test(args[0]))


# TODO write XPASS test that does exit(1)

if __name__ == "__main__":
    try:
        main()
    except Exception as e:
        traceback.print_exc()
        exit(128)                      # Something went wrong with this script
