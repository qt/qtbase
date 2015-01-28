/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/QCoreApplication>
#include <QtTest/QtTest>

#define TESTFILE "testfile"

class FindTestData: public QObject
{
    Q_OBJECT
private slots:
    void initTestCase();
    void cleanupTestCase();

    void paths();

private:
    bool touch(QString const&);
};

void FindTestData::initTestCase()
{
    // verify that our qt.conf is working as expected.
    QString app_path = QCoreApplication::applicationDirPath();
    QString install_path = app_path
#ifdef Q_OS_MAC
        + "/Contents"
#endif
        + "/tests";
    QVERIFY(QDir("/").mkpath(install_path));
    QVERIFY(QDir("/").mkpath(install_path + "/findtestdata"));
    QCOMPARE(QLibraryInfo::location(QLibraryInfo::TestsPath), install_path);

    // make fake source and build directories
    QVERIFY(QDir("/").mkpath(app_path + "/fakesrc"));
    QVERIFY(QDir("/").mkpath(app_path + "/fakebuild"));
}

void FindTestData::cleanupTestCase()
{
    QString app_path = QCoreApplication::applicationDirPath();
    QFile(app_path + "/tests/findtestdata/" TESTFILE).remove();
    QFile(app_path + "/tests/" TESTFILE).remove();
    QFile(app_path + "/fakesrc/" TESTFILE).remove();
    QDir("/").rmpath(app_path + "/tests/findtestdata");
    QDir("/").rmpath(app_path + "/fakesrc");
    QDir("/").rmpath(app_path + "/fakebuild");
}

// Create an empty file at the specified path (or return false)
bool FindTestData::touch(QString const& path)
{
    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning("Failed to create %s: %s", qPrintable(path), qPrintable(file.errorString()));
        return false;
    }
    return true;
}

void FindTestData::paths()
{
    // There are three possible locations for the testdata.
    // In this test, we will put the testdata at all three locations,
    // then remove it from one location at a time and verify that
    // QFINDTESTDATA correctly falls back each time.

    // 1. relative to test binary.
    QString app_path = QCoreApplication::applicationDirPath();
    QString testfile_path1 = app_path + "/" TESTFILE;
    QVERIFY(touch(testfile_path1));

    // 2. at the test install path (faked via qt.conf)
    QString testfile_path2 = app_path
#ifdef Q_OS_MAC
        + "/Contents"
#endif
        + "/tests/findtestdata/" TESTFILE;
    QVERIFY(touch(testfile_path2));

    // 3. at the source path (which we will fake later on)
    QString testfile_path3 = app_path + "/fakesrc/" TESTFILE;
    QVERIFY(touch(testfile_path3));

    // OK, all testdata created.  Verify that they are found in correct priority order.

    QCOMPARE(QFINDTESTDATA(TESTFILE), testfile_path1);
    QVERIFY(QFile(testfile_path1).remove());

    QCOMPARE(QFINDTESTDATA(TESTFILE), testfile_path2);
    QVERIFY(QFile(testfile_path2).remove());

    // We cannot reasonably redefine __FILE__, so we call the underlying function instead.
    // __FILE__ may be absolute or relative path; test both.

    // absolute:
#if defined(Q_OS_WIN)
    QCOMPARE(QTest::qFindTestData(TESTFILE, qPrintable(app_path + "/fakesrc/fakefile.cpp"), __LINE__).toLower(), testfile_path3.toLower());
#else
    QCOMPARE(QTest::qFindTestData(TESTFILE, qPrintable(app_path + "/fakesrc/fakefile.cpp"), __LINE__), testfile_path3);
#endif
    // relative: (pretend that we were compiled within fakebuild directory, pointing at files in ../fakesrc)
#if defined(Q_OS_WIN)
    QCOMPARE(QTest::qFindTestData(TESTFILE, "../fakesrc/fakefile.cpp", __LINE__, qPrintable(app_path + "/fakebuild")).toLower(), testfile_path3.toLower());
#else
    QCOMPARE(QTest::qFindTestData(TESTFILE, "../fakesrc/fakefile.cpp", __LINE__, qPrintable(app_path + "/fakebuild")), testfile_path3);
#endif
    QVERIFY(QFile(testfile_path3).remove());

    // Note, this is expected to generate a warning.
    // We can't use ignoreMessage, because the warning comes from testlib,
    // not via a "normal" qWarning.  But it's OK, our caller (tst_selftests)
    // will verify that the warning is printed.
    QCOMPARE(QFINDTESTDATA(TESTFILE), QString());
}

QTEST_MAIN(FindTestData)
#include "findtestdata.moc"
