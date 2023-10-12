#!/usr/bin/env python3
# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

from argparse import ArgumentParser, RawTextHelpFormatter
import os
import subprocess
import re
import sys


USAGE = """
Regenerate all tests' output.

Usage: cd to the build directory containing the directories with
the subtest binaries, invoke this script; optionally pass the names of sub-dirs
and formats to limit which tests to regenerate expected_* files for.

The saved test output is used by ./tst_selftests.cpp, which compares
it to the output of each test, ignoring various boring changes.
This script canonicalises the parts that would exhibit those boring
changes, so as to avoid noise in git (and conflicts in merges) for
the saved copies of the output.
"""


DEFAULT_FORMATS = ['xml', 'txt', 'junitxml', 'lightxml', 'teamcity', 'tap', 'csv']


TESTS = ['assert', 'badxml', 'benchlibcallgrind', 'benchlibcounting',
         'benchlibeventcounter', 'benchliboptions', 'benchlibtickcounter',
         'benchlibwalltime', 'blacklisted', 'cmptest', 'commandlinedata',
         'counting', 'crashes', 'datatable', 'datetime', 'deleteLater',
         'deleteLater_noApp', 'differentexec', 'eventloop', 'exceptionthrow',
         'expectfail', "extendedcompare", 'failcleanup', 'failcleanuptestcase',
         'faildatatype', 'failfetchtype', 'failinit', 'failinitdata',
         'fetchbogus', 'findtestdata', 'float', 'globaldata', 'longstring',
         'maxwarnings', 'mouse', 'multiexec', 'pairdiagnostics', 'pass',
         'printdatatags', 'printdatatagswithglobaltags', 'qexecstringlist',
         'signaldumper', 'silent', 'singleskip', 'skip', 'skipcleanup',
         'skipcleanuptestcase', 'skipinit', 'skipinitdata', 'sleep', 'strcmp',
         'subtest', 'testlib', 'tuplediagnostics', 'verbose1', 'verbose2',
         'verifyexceptionthrown', 'warnings', 'watchdog', 'junit', 'keyboard']


class Fail (Exception): pass

class Cleaner (object):
    """Tool to clean up test output to make diff-ing runs useful.

    We care about whether tests pass or fail - if that changes,
    something that matters has happened - and we care about some
    changes to what they say when they do fail; but we don't care
    exactly what line of what file the failing line of code now
    occupies, nor do we care how many milliseconds each test took to
    run; and changes to the Qt version number mean nothing to us.

    Create one singleton instance; it'll do mildly expensive things
    once and you can use its .clean() method to tidy up your test
    output."""

    def __init__(self):
        """Set up the details we need for later cleaning.

        Saves the directory of this
        script as self.sourceDir, so client can find tst_selftests.cpp
        there.  Checks here does look as expected in a build tree -
        raising Fail() if not - then retrieves the Qt
        version (saved as .version for the benefit of clients) and
        prepares the sequence of (regex, replace) pairs that .clean()
        needs to do its job."""
        self.version, self.sourceDir, self.__replace = self.__getPatterns()

    @staticmethod
    def _read_qt_version(qtbase_dir):
        cmake_conf_file = os.path.join(qtbase_dir, '.cmake.conf')
        with open(cmake_conf_file) as f:
            for line in f:
                # set(QT_REPO_MODULE_VERSION "6.1.0")
                if 'set(QT_REPO_MODULE_VERSION' in line:
                    return line.strip().split('"')[1]

        raise RuntimeError("Someone broke .cmake.conf formatting again")

    @staticmethod
    def __getPatterns(patterns = (
            # Timings:
            (r'( *<Duration msecs=)"[\d\.]+"/>', r'\1"0"/>'), # xml, lightxml
            (r'(Totals:.*,) *[0-9.]+ms', r'\1 0ms'), # txt
            (r'(<testsuite .*? timestamp=")[^"]+(".*>)', r'\1@TEST_START_TIME@\2'), # junit
            (r'(<(testsuite|testcase) .*? time=")[^"]+(".*>)', r'\1@TEST_DURATION@\3'), # junit
            # Benchmarks:
            (r'[0-9,.]+( (?:CPU ticks|msecs) per iteration \(total:) [0-9,.]+ ', r'0\1 0, '), # txt
            (r'(<BenchmarkResult metric="(?:CPUTicks|WalltimeMilliseconds)".*\bvalue=)"[^"]+"', r'\1"0"'), # xml, lightxml
            # Build details:
            (r'(Config: Using QtTest library).*', r'\1'), # txt
            (r'( *<QtBuild)>[^<]+</QtBuild>', r'\1/>'), # xml, lightxml
            (r'(<property name="QtBuild" value=")[^"]+"', r'\1"'), # junitxml
            (r'(<testsuite .*? hostname=")[^"]+(".*>)', r'\1@HOSTNAME@\2'), # junit
            # Line numbers in source files:
            (r'(ASSERT: ("|&quot;).*("|&quot;) in file .*, line) \d+', r'\1 0'), # lightxml
            (r'(Loc: \[[^[\]()]+)\(\d+\)', r'\1(0)'), # txt
            (r'(\[Loc: [^[\]()]+)\(\d+\)', r'\1(0)'), # teamcity
            (r'(<(?:Incident|Message)\b.*\bfile=.*\bline=)"\d+"', r'\1"0"'), # lightxml, xml
            (r'(at: .*?):\d+\)', r'\1:0)'), # tap
            (r'(line:) \d+', r'\1 0'), # tap
            # Pointers printed by signal dumper:
            (r'\(\b[a-f0-9]{8,}\b\)', r'(_POINTER_)'),
            # Example/for reference:
            # ((QString&)@55f5fbb8dd40)
            # ((const QVector<int>*)7ffd671d4558)
            (r'\((\((?:const )?\w+(?:<[^>]+>)?[*&]*\)@?)\b[a-f\d]{8,}\b\)', r'(\1_POINTER_)'),
            # For xml output there is no '<', '>' or '&', so we need an alternate version for that:
            # ((QVector&lt;int&gt;&amp;)@5608b455e640)
            (r'\((\((?:const )?\w+(?:&lt;(?:[^&]|&(?!gt;))*&gt;)?(?:\*|&amp;)?\)@?)[a-z\d]+\b\)', r'(\1_POINTER_)'),
            # QEventDispatcher{Glib,Win32,etc.}
            (r'\bQEventDispatcher\w+\b', r'QEventDispatcherPlatform'),
            ),
                      precook = re.compile):
        """Private implementation details of __init__()."""

        # Are we being run from the right place ?
        scriptPath = os.path.dirname(os.path.abspath(__file__))
        hereNames, depth = scriptPath.split(os.path.sep), 5
        hereNames = hereNames[-depth:] # path components from qtbase down
        assert hereNames[0] == 'qtbase', ('Script moved: please correct depth', hereNames)
        qtbase_dir = os.path.realpath(os.path.join(scriptPath, '..', '..', '..', '..'))
        qtver = Cleaner._read_qt_version(qtbase_dir)
        hereNames = tuple(hereNames)
        # Add path to specific sources and to tst_*.cpp if missing (for in-source builds):
        patterns += ((r'(^|[^/])\b(qtestcase.cpp)\b', r'\1qtbase/src/testlib/\2'),
                     # Add more special cases here, if they show up !
                     (r'([\[" ])\.\./(counting/tst_counting.cpp)\b',
                      r'\1' + os.path.sep.join(hereNames + (r'\2',))),
                     # The common pattern:
                     (r'(^|[^/])\b(tst_)?([a-z]+\d*)\.cpp\b',
                      r'\1' + os.path.sep.join(hereNames + (r'\3', r'\2\3.cpp'))))

        sentinel = os.path.sep + hereNames[0] + os.path.sep # '/qtbase/'
        # Identify the path prefix of our qtbase ancestor directory
        # (source, build and $PWD, when different); trim such prefixes
        # off all paths we see.
        roots = tuple(r[:r.find(sentinel) + 1].encode('unicode-escape').decode('utf-8')
                      for r in set((os.getcwd(), scriptPath, os.environ.get('PWD', '')))
                      if sentinel in r)
        patterns += tuple((root, r'') for root in roots) + (
            (r'\.'.join(qtver.split('.')), r'@INSERT_QT_VERSION_HERE@'),)
        if any('-' in r for r in roots):
            # Our xml formats replace hyphens with a character entity:
            patterns += tuple((root.replace('-', '&#x0*2D;'), r'')
                              for root in roots if '-' in root)

        return qtver, scriptPath, tuple((precook(p), r) for p, r in patterns)

    def clean(self, data):
        """Remove volatile details from test output.

        Takes the full test output as a single (possibly huge)
        multi-line string; iterates over cleaned lines of output."""
        for line in data.split('\n'):
            # Replace all occurrences of each regex:
            for searchRe, replaceExp in self.__replace:
                line = searchRe.sub(replaceExp, line)
            yield line

class Scanner (object):
    """Knows which subdirectories to generate output for.

    Tell its constructor the name of this source directory (see
    Cleaner's .sourceDir) and it'll scan tst_selftests.cpp for the
    list.  Its .subdirs() can then filter a user-supplied list of
    subdirs or generate the full list, when the user supplied
    none."""
    def __init__(self):
        pass

    def subdirs(self, given, skip_callgrind=False):
        if given:
            for d in given:
                if not os.path.isdir(d):
                    print('No such directory:', d, '- skipped')
                elif skip_callgrind and d == 'benchlibcallgrind':
                    pass # Skip this test, as requeted.
                elif d in TESTS:
                    yield d
                else:
                    print(f'Directory {d} is not in the list of tests')
        else:
            tests = TESTS
            if skip_callgrind:
                tests.remove('benchlibcallgrind')
            missing = 0
            for d in tests:
                if os.path.isdir(d):
                    yield d
                else:
                    missing += 1
                    print(f"directory {d} doesn't exist, was it removed?")
            if missing == len(tests):
                print(USAGE)

del re

# Keep in sync with tst_selftests.cpp's testEnvironment():
def baseEnv(platname=None,
            keep=('PATH', 'QT_QPA_PLATFORM', 'ASAN_OPTIONS'),
            posix=('HOME', 'USER', 'QEMU_SET_ENV', 'QEMU_LD_PREFIX'),
            nonapple=('DISPLAY', 'XAUTHORITY', 'XAUTHLOCALHOSTNAME'), # and XDG_*
            # Don't actually know how to test for QNX, so this is ignored:
            qnx=('GRAPHICS_ROOT', 'TZ'),
            # Probably not actually relevant
            preserveLib=('QT_PLUGIN_PATH', 'LD_LIBRARY_PATH'),
            # Shall be modified on first call (a *copy* is returned):
            cached={}):
    """Lazily-evaluated standard environment for sub-tests to run in.

    This prunes the parent process environment, selecting a only those
    variables we chose to keep.  The platname passed to the first call
    helps select which variables to keep.  The environment computed
    then is cached: a copy of this is returned on that call and each
    subsequent call.\n"""

    if not cached:
        xdg = False
        # The platform module may be more apt for the platform tests here.
        if os.name == 'posix':
            keep += posix
            if platname != 'darwin':
                keep += nonapple
                xdg = True
        if 'QT_PRESERVE_TESTLIB_PATH' in os.environ:
            keep += preserveLib

        cached = dict(
            LC_ALL = 'en-US.UTF-8', # Use standard locale
            # Avoid interference from any qtlogging.ini files, e.g. in
            # /etc/xdg/QtProject/, (must match tst_selftests.cpp's
            # processEnvironment()'s value):
            QT_LOGGING_RULES = '*.debug=true;qt.*=false')

        for k, v in os.environ.items():
            if k in keep or (xdg and k.startswith('XDG_')):
                cached[k] = v

    return cached.copy()

def testEnv(testname,
            # Make sure this matches tst_Selftests::doRunSubTest():
            extraEnv = {
        "crashers": { "QTEST_DISABLE_CORE_DUMP": "1",
                      "QTEST_DISABLE_STACK_DUMP": "1" },
        "watchdog": { "QTEST_FUNCTION_TIMEOUT": "100" },
        },
            # Must match tst_Selftests::runSubTest_data():
            crashers = ("assert", "crashes", "crashedterminate",
                        "exceptionthrow", "faildatatype", "failfetchtype",
                        "fetchbogus", "silent", "watchdog")):
    """Determine the environment in which to run a test."""
    data = baseEnv()
    if testname in crashers:
        data.update(extraEnv["crashers"])
    if testname in extraEnv:
        data.update(extraEnv[testname])
    return data

def shouldIgnoreTest(testname, format):
    """Test whether to exclude a test/format combination.

    See TestLogger::shouldIgnoreTest() in tst_selftests.cpp; it starts
    with various exclusions for opt-in tests, platform dependencies
    and tool availability; we ignore those, as we need the test data
    to be present when those exclusions aren't in effect.

    In the remainder, exclude what it always excludes.
    """
    if format != 'txt':
        if testname in ("differentexec",
                        "multiexec",
                        "qexecstringlist",
                        "benchliboptions",
                        "printdatatags",
                        "printdatatagswithglobaltags",
                        "silent",
                        "crashes",
                        "benchlibcallgrind",
                        "float",
                        "sleep"):
            return True

    if testname == "badxml" and not format.endswith('xml'):
        return True

    # Skip benchlib* for teamcity, and everything else for csv:
    if format == ('teamcity' if testname.startswith('benchlib') else 'csv'):
        return True

    if testname == "junit" and format != "junitxml":
        return True

    return False

def generateTestData(test_path, expected_path, clean, formats):
    """Run one test and save its cleaned results.

    Required arguments are the path to test directory (the binary
    it contains is expected to have the same name), a function
    that'll clean a test-run's output; see Cleaner.clean() and a list of
    formats.
    """
    # MS-Win: shall need to add .exe to this
    testname = os.path.basename(test_path)
    path = os.path.join(test_path, testname)
    if not os.path.isfile(path):
        print("Warning: directory", testname, "contains no test executable")
        return

    # Prepare environment in which to run tests:
    env = testEnv(testname)

    for format in formats:
        if shouldIgnoreTest(testname, format):
            continue
        print(f'  running {testname}/{format}')
        cmd = [path, f'-{format}']
        expected_file = f'expected_{testname}.{format}'
        data = subprocess.Popen(cmd, stdout=subprocess.PIPE, env=env,
                                universal_newlines=True).communicate()[0]
        with open(os.path.join(expected_path, expected_file), 'w') as out:
            out.write('\n'.join(clean(data))) # write() appends a newline, too

def main(argv):
    """Argument parsing and driver for the real work"""
    argument_parser = ArgumentParser(description=USAGE, formatter_class=RawTextHelpFormatter)
    argument_parser.add_argument('--formats', '-f',
                                 help='Comma-separated list of formats')
    argument_parser.add_argument('--skip-callgrind', '-s', action='store_true',
                                 help='Skip the (no longer expensive) benchlib callgrind test')
    argument_parser.add_argument('subtests', help='subtests to regenerate',
                                 nargs='*', type=str)

    options = argument_parser.parse_args(argv[1:])
    formats = options.formats.split(',') if options.formats else DEFAULT_FORMATS

    cleaner = Cleaner()
    src_dir = cleaner.sourceDir

    if not options.skip_callgrind:
        # Skip it, even if not requested, when valgrind isn't available:
        try:
            probe = subprocess.Popen(['valgrind', '--version'], stdout=subprocess.PIPE,
                                     env=testEnv('benchlibcallgrind'), universal_newlines=True)
        except FileNotFoundError:
            options.skip_callgrind = True
            print("Failed to find valgrind, skipping benchlibcallgrind test")

    tests = tuple(Scanner().subdirs(options.subtests, options.skip_callgrind))
    print("Generating", len(tests), "test results for", cleaner.version, "in:", src_dir)
    for path in tests:
        generateTestData(path, src_dir, cleaner.clean, formats)

if __name__ == '__main__':
    # Executed when script is run, not when imported (e.g. to debug)
    baseEnv(sys.platform) # initializes its cache

    if sys.platform.startswith('win'):
        print("This script does not work on Windows.")
        exit()

    try:
        main(sys.argv)
    except Fail as what:
        sys.stderr.write('Failed: ' + ' '.join(what.args) + '\n')
        exit(1)
