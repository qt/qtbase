#!/usr/bin/env python3
#############################################################################
##
## Copyright (C) 2016 The Qt Company Ltd.
## Contact: https://www.qt.io/licensing/
##
## This file is part of the release tools of the Qt Toolkit.
##
## $QT_BEGIN_LICENSE:GPL-EXCEPT$
## Commercial License Usage
## Licensees holding valid commercial Qt licenses may use this file in
## accordance with the commercial license agreement provided with the
## Software or, alternatively, in accordance with the terms contained in
## a written agreement between you and The Qt Company. For licensing terms
## and conditions see https://www.qt.io/terms-conditions. For further
## information use the contact form at https://www.qt.io/contact-us.
##
## GNU General Public License Usage
## Alternatively, this file may be used under the terms of the GNU
## General Public License version 3 as published by the Free Software
## Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
## included in the packaging of this file. Please review the following
## information to ensure the GNU General Public License requirements will
## be met: https://www.gnu.org/licenses/gpl-3.0.html.
##
## $QT_END_LICENSE$
##
#############################################################################

# Regenerate all tests' output.
#
# Usage: cd to the build directory corresponding to this script's
# location; invoke this script; optionally pass the names of sub-dirs
# to limit which tests to regenerate expected_* files for.
#
# The saved test output is used by ./tst_selftests.cpp, which compares
# it to the output of each test, ignoring various boring changes.
# This script canonicalises the parts that would exhibit those boring
# changes, so as to avoid noise in git (and conflicts in merges) for
# the saved copies of the output.  If you add or remove any files, be
# sure to update selftests.qrc to match; the selftest only sees files
# listed there.

import os
import subprocess
import re

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

    def __init__(self, here, command):
        """Set up the details we need for later cleaning.

        Takes two parameters: here is os.getcwd() and command is how
        this script was invoked, from which we'll work out where it
        is; in a shadow build, the former is the build tree's location
        corresponding to this last.  Saves the directory of this
        script as self.sourceDir, so client can find tst_selftests.cpp
        there.  Checks here does look as expected in a build tree -
        raising Fail() if not - then invokes qmake to discover Qt
        version (saved as .version for the benefit of clients) and
        prepares the sequence of (regex, replace) pairs that .clean()
        needs to do its job."""
        self.version, self.sourceDir, self.__replace = self.__getPatterns(here, command)

    @staticmethod
    def __getPatterns(here, command,
                      patterns = (
            # Timings:
            (r'( *<Duration msecs=)"[\d\.]+"/>', r'\1"0"/>'), # xml, lightxml
            (r'(Totals:.*,) *[0-9.]+ms', r'\1 0ms'), # txt
            # Benchmarks:
            (r'[0-9,.]+( (?:CPU ticks|msecs) per iteration \(total:) [0-9,.]+ ', r'0\1 0, '), # txt
            (r'(<BenchmarkResult metric="(?:CPUTicks|WalltimeMilliseconds)".*\bvalue=)"[^"]+"', r'\1"0"'), # xml, lightxml
            # Build details:
            (r'(Config: Using QtTest library).*', r'\1'), # txt
            (r'( *<QtBuild)>[^<]+</QtBuild>', r'\1/>'), # xml, lightxml
            (r'(<property value=")[^"]+(" name="QtBuild"/>)', r'\1\2'), # xunitxml
            # Line numbers in source files:
            (r'(ASSERT: ".*" in file .*, line) \d+', r'\1 0'), # lightxml
            (r'(Loc: \[[^[\]()]+)\(\d+\)', r'\1(0)'), # txt
            (r'(\[Loc: [^[\]()]+)\(\d+\)', r'\1(0)'), # teamcity
            (r'(<(?:Incident|Message)\b.*\bfile=.*\bline=)"\d+"', r'\1"0"'), # lightxml, xml
            ),
                      precook = re.compile):
        """Private implementation details of __init__()."""

        qmake = ('..',) * 4 + ('bin', 'qmake')
        qmake = os.path.join(*qmake)

        if os.path.sep in command:
            scriptPath = os.path.abspath(command)
        elif os.path.exists(command):
            # e.g. if you typed "python3 generate_expected_output.py"
            scriptPath = os.path.join(here, command)
        else:
            # From py 3.2: could use os.get_exec_path() here.
            for d in os.environ.get('PATH', '').split(os.pathsep):
                scriptPath = os.path.join(d, command)
                if os.path.isfile(scriptPath):
                    break
            else: # didn't break
                raise Fail('Unable to find', command, 'in $PATH')

        # Are we being run from the right place ?
        myNames = scriptPath.split(os.path.sep)
        if not (here.split(os.path.sep)[-5:] == myNames[-6:-1]
                and os.path.isfile(qmake)):
            raise Fail('Run', myNames[-1], 'in its directory of a completed build')

        try:
            qtver = subprocess.check_output([qmake, '-query', 'QT_VERSION'])
        except OSError as what:
            raise Fail(what.strerror)
        qtver = qtver.strip().decode('utf-8')

        scriptPath = os.path.dirname(scriptPath) # ditch leaf file-name
        sentinel = os.path.sep + 'qtbase' + os.path.sep # '/qtbase/'
        # Identify the path prefix of our qtbase ancestor directory
        # (source, build and $PWD, when different); trim such prefixes
        # off all paths we see.
        roots = tuple(r[:r.find(sentinel) + 1].encode('unicode-escape').decode('utf-8')
                      for r in set((here, scriptPath, os.environ.get('PWD', '')))
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
    def __init__(self, srcDir):
        self.__tested = tuple(self.__scan_cpp(os.path.join(srcDir, 'tst_selftests.cpp')))

    @staticmethod
    def __scan_cpp(name,
                   trimc = re.compile(r'/\*.*?\*/').sub,
                   trimcpp = re.compile(r'//.*$').sub,
                   first = re.compile(r'(QStringList|auto)\s+tests\s*=\s*QStringList\(\)').match,
                   match = re.compile(r'(?:tests\s*)?<<\s*"(\w+)"').match,
                   last = re.compile(r'\bfor.*\b(LoggerSet|auto)\b.*\ballLoggerSets\(\)').search):
        """Scans tst_selftests.cpp to find which subdirs matter.

        There's a list, tests, to which all subdir names get added, if
        they're to be tested.  Other sub-dirs aren't tested, so
        there's no sense in generating output for them."""
        scan = False
        with open(name) as src:
            for line in src:
                line = trimcpp('', trimc('', line.strip())).strip()
                if not scan:
                    got = first(line)
                    if got:
                        scan, line = True, line[len(got.group()):]
                if scan:
                    if last(line): break
                    got = match(line)
                    while got:
                        yield got.group(1)
                        line = line[len(got.group()):].strip()
                        got = match(line)

    def subdirs(self, given):
        if given:
            for d in given:
                if not os.path.isdir(d):
                    print('No such directory:', d, '- skipped')
                elif d in self.__tested:
                    yield d
                else:
                    print('Directory', d, 'is not tested by tst_selftests.cpp')
        else:
            for d in self.__tested:
                if os.path.isdir(d):
                    yield d
                else:
                    print('tst_selftests.cpp names', d, "as a test, but it doesn't exist")
del re

def generateTestData(testname, clean,
                     formats = ('xml', 'txt', 'xunitxml', 'lightxml', 'teamcity'),
                     extraArgs = {
        "commandlinedata": "fiveTablePasses fiveTablePasses:fiveTablePasses_data1 -v2",
        "benchlibcallgrind": "-callgrind",
        "benchlibeventcounter": "-eventcounter",
        "benchliboptions": "-eventcounter",
        "benchlibtickcounter": "-tickcounter",
        "badxml": "-eventcounter",
        "benchlibcounting": "-eventcounter",
        "printdatatags": "-datatags",
        "printdatatagswithglobaltags": "-datatags",
        "silent": "-silent",
        "verbose1": "-v1",
        "verbose2": "-v2",
        }):
    """Run one test and save its cleaned results.

    Required arguments are the name of the test directory (the binary
    it contains is expected to have the same name) and a function
    that'll clean a test-run's output; see Cleaner.clean().
    """
    # MS-Win: shall need to add .exe to this
    path = os.path.join(testname, testname)
    if not os.path.isfile(path):
        print("Warning: directory", testname, "contains no test executable")
        return

    print("  running", testname)
    for format in formats:
        cmd = [path, '-' + format]
        if testname in extraArgs:
            cmd += extraArgs[testname].split()

        data = subprocess.Popen(cmd, stdout=subprocess.PIPE,
                                universal_newlines=True).communicate()[0]
        with open('expected_' + testname + '.' + format, 'w') as out:
            out.write('\n'.join(clean(data))) # write() appends a newline, too

def main(name, *args):
    """Minimal argument parsing and driver for the real work"""
    os.environ.update(
        LC_ALL = 'C', # Use standard locale
        # Avoid interference from any qtlogging.ini files, e.g. in
        # /etc/xdg/QtProject/, (must match tst_selftests.cpp's
        # processEnvironment()'s value):
        QT_LOGGING_RULES = '*.debug=true;qt.*=false')

    herePath = os.getcwd()
    cleaner = Cleaner(herePath, name)

    tests = tuple(Scanner(cleaner.sourceDir).subdirs(args))
    print("Generating", len(tests), "test results for", cleaner.version, "in:", herePath)
    for path in tests:
        generateTestData(path, cleaner.clean)

if __name__ == '__main__':
    # Executed when script is run, not when imported (e.g. to debug)
    import sys

    if sys.platform.startswith('win'):
        print("This script does not work on Windows.")
        exit()

    try:
        main(*sys.argv)
    except Fail as what:
        sys.stderr.write('Failed: ' + ' '.join(what.args) + '\n')
        exit(1)
