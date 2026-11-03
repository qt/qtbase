#!/usr/bin/env python3
# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

import os
import shutil
import subprocess
import sys
import unittest

from pathlib import Path
from tempfile import TemporaryDirectory

my_dir = Path(__file__).resolve().parent
script = my_dir.parents[3] / "util" / "testrunner" / "qt-modified-testfunctions.py"

TEST_SOURCE = """\
#include <QTest>

class tst_Foo : public QObject
{
    Q_OBJECT
private slots:
    void alpha_data();
    void alpha();
    void beta();
};

void tst_Foo::alpha_data()
{
    QTest::addColumn<int>("i");
}

void tst_Foo::alpha()
{
    QFETCH(int, i);
    QCOMPARE(i, @ALPHA@);
}

void tst_Foo::beta()
{
    QVERIFY(@BETA@);
}

QTEST_MAIN(tst_Foo)
#include "tst_foo.moc"
"""


@unittest.skipIf(shutil.which("git") is None, "git is not available")
class TestModifiedTestFunctions(unittest.TestCase):

    def setUp(self):
        self.tempdir = TemporaryDirectory(prefix="tst_qt_modified_testfunctions-")
        self.repo = Path(self.tempdir.name)
        self.git("init", "-q")
        self.git("config", "user.name", "Test")
        self.git("config", "user.email", "test@example.com")
        self.commit_source(alpha="1", beta="true", message="Base")
        self.base = self.git("rev-parse", "HEAD").strip()

    def tearDown(self):
        self.tempdir.cleanup()

    def git(self, *args) -> str:
        return subprocess.run(["git", *args], cwd=self.repo, check=True,
                              stdout=subprocess.PIPE, universal_newlines=True).stdout

    def commit_source(self, alpha: str, beta: str, message: str):
        source = TEST_SOURCE.replace("@ALPHA@", alpha).replace("@BETA@", beta)
        (self.repo / "tst_foo.cpp").write_text(source)
        self.git("add", "tst_foo.cpp")
        self.git("commit", "-q", "-m", message)

    def modified_functions(self):
        output = self.repo / "modified.txt"
        env = {k: v for k, v in os.environ.items() if k != "COIN_CONFIGURE_ARGS"}
        proc = subprocess.run([sys.executable, str(script), "--range", f"{self.base}..HEAD",
                               "--output", str(output)],
                              cwd=self.repo, env=env, stdout=subprocess.PIPE,
                              stderr=subprocess.PIPE, universal_newlines=True)
        self.assertEqual(proc.returncode, 0, proc.stderr)
        if not output.exists():
            return [], proc.stderr
        return output.read_text().split(), proc.stderr

    def test_no_footer(self):
        self.commit_source(alpha="2", beta="false", message="Change both")
        functions, _ = self.modified_functions()
        self.assertEqual(functions, ["tst_Foo::alpha", "tst_Foo::beta"])

    def test_footer_exempts_function(self):
        self.commit_source(alpha="2", beta="false",
                           message="Change both\n\nFlaky-test-function: tst_Foo::beta (QTBUG-1)\n")
        functions, _ = self.modified_functions()
        self.assertEqual(functions, ["tst_Foo::alpha"])

    def test_footer_exempts_all_functions(self):
        self.commit_source(alpha="2", beta="false",
                           message="Change both\n\n"
                                   "Flaky-test-function: tst_Foo::alpha\n"
                                   "flaky-test-function: tst_Foo::beta()\n")
        functions, _ = self.modified_functions()
        self.assertEqual(functions, [])

    def test_footer_exempts_data_function(self):
        source = TEST_SOURCE.replace('addColumn<int>("i")', 'addColumn<int>("j")')
        source = source.replace("@ALPHA@", "1").replace("@BETA@", "true")
        (self.repo / "tst_foo.cpp").write_text(source)
        self.git("commit", "-q", "-a", "-m",
                 "Change data\n\nFlaky-test-function: tst_Foo::alpha\n")
        functions, _ = self.modified_functions()
        self.assertEqual(functions, [])

    def test_footer_only_applies_to_its_commit(self):
        self.commit_source(alpha="2", beta="true", message="Change alpha")
        self.commit_source(alpha="3", beta="true",
                           message="Change alpha again\n\nFlaky-test-function: tst_Foo::alpha\n")
        functions, _ = self.modified_functions()
        self.assertEqual(functions, ["tst_Foo::alpha"])

    def test_footer_for_unmodified_function_warns(self):
        self.commit_source(alpha="2", beta="true",
                           message="Change alpha\n\nFlaky-test-function: tst_Foo::beta\n")
        functions, stderr = self.modified_functions()
        self.assertEqual(functions, ["tst_Foo::alpha"])
        self.assertIn("tst_Foo::beta, which it doesn't modify", stderr)


if __name__ == "__main__":
    unittest.main()
