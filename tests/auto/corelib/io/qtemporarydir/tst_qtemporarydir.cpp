/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtTest/QtTest>
#include <qcoreapplication.h>
#include <qstring.h>
#include <qtemporarydir.h>
#include <qfile.h>
#include <qdir.h>
#include <qset.h>
#ifdef Q_OS_WIN
# include <windows.h>
#endif
#ifdef Q_OS_UNIX // for geteuid()
# include <sys/types.h>
# include <unistd.h>
#endif

class tst_QTemporaryDir : public QObject
{
    Q_OBJECT
public:
public slots:
    void initTestCase();
    void cleanupTestCase();

private slots:
    void construction();
    void fileTemplate();
    void fileTemplate_data();
    void getSetCheck();
    void fileName();
    void autoRemove();
    void nonWritableCurrentDir();
    void openOnRootDrives();
    void stressTest();
    void rename();

    void QTBUG_4796_data();
    void QTBUG_4796();

public:
};

void tst_QTemporaryDir::initTestCase()
{
    QVERIFY(QDir("test-XXXXXX").exists() || QDir().mkdir("test-XXXXXX"));
    QCoreApplication::setApplicationName("tst_qtemporarydir");
}

void tst_QTemporaryDir::cleanupTestCase()
{
    QVERIFY(QDir().rmdir("test-XXXXXX"));
}

void tst_QTemporaryDir::construction()
{
    QTemporaryDir dir;
    QString tmp = QDir::tempPath();
    QCOMPARE(dir.path().left(tmp.size()), tmp);
    QVERIFY(dir.path().contains("tst_qtemporarydir"));
    QVERIFY(QFileInfo(dir.path()).isDir());
}

// Testing get/set functions
void tst_QTemporaryDir::getSetCheck()
{
    QTemporaryDir obj1;
    // bool QTemporaryDir::autoRemove()
    // void QTemporaryDir::setAutoRemove(bool)
    obj1.setAutoRemove(false);
    QCOMPARE(false, obj1.autoRemove());
    obj1.setAutoRemove(true);
    QCOMPARE(true, obj1.autoRemove());
}

void tst_QTemporaryDir::fileTemplate_data()
{
    QTest::addColumn<QString>("constructorTemplate");
    QTest::addColumn<QString>("prefix");

    QTest::newRow("constructor default") << "" << "tst_qtemporarydir-";

    QTest::newRow("constructor with xxx sufix") << "qt_XXXXXXxxx" << "qt_XXXXXXxxx";
    QTest::newRow("constructor with xXx sufix") << "qt_XXXXXXxXx" << "qt_XXXXXXxXx";
    QTest::newRow("constructor with no suffix") << "qt_XXXXXX" << "qt_";
    QTest::newRow("constructor with >6 X's, no suffix") << "qt_XXXXXXXXXX" << "qt_";
    // When more than 6 X are present at the end, linux and windows will only replace the last 6,
    // while Mac OS will actually replace all of them so we can only expect "qt_" (and check isValid).
    QTest::newRow("constructor with XXXX suffix") << "qt_XXXXXX_XXXX" << "qt_";
    QTest::newRow("constructor with XXXX prefix") << "qt_XXXX" << "qt_";
    QTest::newRow("constructor with XXXXX prefix") << "qt_XXXXX" << "qt_";
}

void tst_QTemporaryDir::fileTemplate()
{
    QFETCH(QString, constructorTemplate);
    QFETCH(QString, prefix);

    QTemporaryDir tempDir(constructorTemplate);

    QVERIFY(tempDir.isValid());

    QString dirName = QDir(tempDir.path()).dirName();
    if (prefix.length())
        QCOMPARE(dirName.left(prefix.length()), prefix);
}


/*
    This tests whether the temporary dir really gets placed in QDir::tempPath
*/
void tst_QTemporaryDir::fileName()
{
    // Get QDir::tempPath and make an absolute path.
    QString tempPath = QDir::tempPath();
    QString absoluteTempPath = QDir(tempPath).absolutePath();
    QTemporaryDir dir;
    dir.setAutoRemove(true);
    QString fileName = dir.path();
    QVERIFY2(fileName.contains("/tst_qtemporarydir-"), qPrintable(fileName));
    QVERIFY(QDir(fileName).exists());
    // Get path to the temp dir, without the file name.
    QString absoluteFilePath = QFileInfo(fileName).absolutePath();
#if defined(Q_OS_WIN)
    absoluteFilePath = absoluteFilePath.toLower();
    absoluteTempPath = absoluteTempPath.toLower();
#endif
    QCOMPARE(absoluteFilePath, absoluteTempPath);
}

void tst_QTemporaryDir::autoRemove()
{
    // Test auto remove
    QString dirName;
    {
        QTemporaryDir dir("tempXXXXXX");
        dir.setAutoRemove(true);
        QVERIFY(dir.isValid());
        dirName = dir.path();
    }
#ifdef Q_OS_WIN
    // Windows seems unreliable here: sometimes it says the directory still exists,
    // immediately after we deleted it.
    QTRY_VERIFY(!QDir(dirName).exists());
#else
    QVERIFY(!QDir(dirName).exists());
#endif

    // Test if disabling auto remove works.
    {
        QTemporaryDir dir("tempXXXXXX");
        dir.setAutoRemove(false);
        QVERIFY(dir.isValid());
        dirName = dir.path();
    }
    QVERIFY(QDir(dirName).exists());
    QVERIFY(QDir().rmdir(dirName));
    QVERIFY(!QDir(dirName).exists());

    // Do not explicitly call setAutoRemove (tests if it really is the default as documented)
    {
        QTemporaryDir dir("tempXXXXXX");
        QVERIFY(dir.isValid());
        dirName = dir.path();
    }
#ifdef Q_OS_WIN
    QTRY_VERIFY(!QDir(dirName).exists());
#else
    QVERIFY(!QDir(dirName).exists());
#endif

    // Test autoremove with files and subdirs in the temp dir
    {
        QTemporaryDir tempDir("tempXXXXXX");
        QVERIFY(tempDir.isValid());
        dirName = tempDir.path();
        QDir dir(dirName);
        QVERIFY(dir.mkdir(QString::fromLatin1("dir1")));
        QVERIFY(dir.mkdir(QString::fromLatin1("dir2")));
        QVERIFY(dir.mkdir(QString::fromLatin1("dir2/nested")));
        QFile file(dirName + "/dir1/file");
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write("Hello"), 5LL);
    }
#ifdef Q_OS_WIN
    QTRY_VERIFY(!QDir(dirName).exists());
#else
    QVERIFY(!QDir(dirName).exists());
#endif
}

void tst_QTemporaryDir::nonWritableCurrentDir()
{
#ifdef Q_OS_UNIX
    if (::geteuid() == 0)
        QSKIP("not valid running this test as root");

    struct ChdirOnReturn
    {
        ChdirOnReturn(const QString& d) : dir(d) {}
        ~ChdirOnReturn() {
            QDir::setCurrent(dir);
        }
        QString dir;
    };
    ChdirOnReturn cor(QDir::currentPath());

    QDir::setCurrent("/home");
    // QTemporaryDir("tempXXXXXX") is probably a bad idea in any app
    // where the current dir could anything...
    QTemporaryDir dir("tempXXXXXX");
    dir.setAutoRemove(true);
    QVERIFY(!dir.isValid());
    QVERIFY(dir.path().isEmpty());
#endif
}

void tst_QTemporaryDir::openOnRootDrives()
{
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    unsigned int lastErrorMode = SetErrorMode(SEM_FAILCRITICALERRORS);
#endif
    // If it's possible to create a file in the root directory, it
    // must be possible to create a temp dir there too.
    foreach (const QFileInfo &driveInfo, QDir::drives()) {
        QFile testFile(driveInfo.filePath() + "XXXXXX");
        if (testFile.open(QIODevice::ReadWrite)) {
            testFile.remove();
            QTemporaryDir dir(driveInfo.filePath() + "XXXXXX");
            dir.setAutoRemove(true);
            QVERIFY(dir.isValid());
        }
    }
#if defined(Q_OS_WIN) && !defined(Q_OS_WINCE) && !defined(Q_OS_WINRT)
    SetErrorMode(lastErrorMode);
#endif
}

void tst_QTemporaryDir::stressTest()
{
    const int iterations = 1000;
    QTemporaryDir rootDir;
    QVERIFY(rootDir.isValid());

    QSet<QString> names;
    const QString pattern = rootDir.path() + QStringLiteral("/XXXXXX");
    for (int i = 0; i < iterations; ++i) {
        QTemporaryDir dir(pattern);
        dir.setAutoRemove(false);
        QVERIFY2(dir.isValid(), qPrintable(QString::fromLatin1("Failed to create #%1 under %2.").arg(i).arg(QDir::toNativeSeparators(pattern))));
        QVERIFY(!names.contains(dir.path()));
        names.insert(dir.path());
    }
}

void tst_QTemporaryDir::rename()
{
    // This test checks what happens if the temporary dir is renamed.
    // Then the autodelete feature can't possibly find it.

    QDir dir;
    QVERIFY(!dir.exists("temporary-dir.renamed"));

    QString tempname;
    {
        QTemporaryDir tempDir(dir.filePath("temporary-dir.XXXXXX"));

        QVERIFY(tempDir.isValid());
        tempname = tempDir.path();

        QVERIFY(QDir().rename(tempname, "temporary-dir.renamed"));
        QVERIFY(!QDir(tempname).exists());
        dir.setPath("temporary-dir.renamed");
        QCOMPARE(dir.path(), QString("temporary-dir.renamed"));
        QVERIFY(dir.exists());
    }

    // Auto-delete couldn't find it
    QVERIFY(dir.exists());
    // Clean up by hand
    QVERIFY(dir.removeRecursively());
    QVERIFY(!dir.exists());
}

void tst_QTemporaryDir::QTBUG_4796_data()
{
    QTest::addColumn<QString>("prefix");
    QTest::addColumn<QString>("suffix");
    QTest::addColumn<bool>("openResult");

    QString unicode = QString::fromUtf8("\xc3\xa5\xc3\xa6\xc3\xb8");

    QTest::newRow("<empty>") << QString() << QString() << true;
    QTest::newRow(".") << QString(".") << QString() << true;
    QTest::newRow("..") << QString("..") << QString() << true;
    QTest::newRow("blaXXXXXX") << QString("bla") << QString() << true;
    QTest::newRow("does-not-exist/qt_temp.XXXXXX") << QString("does-not-exist/qt_temp") << QString() << false;
    QTest::newRow("XXXXXX<unicode>") << QString() << unicode << true;
    QTest::newRow("<unicode>XXXXXX") << unicode << QString() << true;
}

void tst_QTemporaryDir::QTBUG_4796() // unicode support
{
    QVERIFY(QDir("test-XXXXXX").exists());

    struct CleanOnReturn
    {
        ~CleanOnReturn()
        {
            foreach (const QString &tempName, tempNames)
                QVERIFY(QDir(tempName).removeRecursively());
        }

        void reset()
        {
            tempNames.clear();
        }

        QStringList tempNames;
    };

    CleanOnReturn cleaner;

    QFETCH(QString, prefix);
    QFETCH(QString, suffix);
    QFETCH(bool, openResult);

    {
        QString fileTemplate1 = prefix + QString("XX") + suffix;
        QString fileTemplate2 = prefix + QString("XXXX") + suffix;
        QString fileTemplate3 = prefix + QString("XXXXXX") + suffix;
        QString fileTemplate4 = prefix + QString("XXXXXXXX") + suffix;

        QTemporaryDir dir1(fileTemplate1);
        QTemporaryDir dir2(fileTemplate2);
        QTemporaryDir dir3(fileTemplate3);
        QTemporaryDir dir4(fileTemplate4);
        QTemporaryDir dir5("test-XXXXXX/" + fileTemplate1);
        QTemporaryDir dir6("test-XXXXXX/" + fileTemplate3);

        QCOMPARE(dir1.isValid(), openResult);
        QCOMPARE(dir2.isValid(), openResult);
        QCOMPARE(dir3.isValid(), openResult);
        QCOMPARE(dir4.isValid(), openResult);
        QCOMPARE(dir5.isValid(), openResult);
        QCOMPARE(dir6.isValid(), openResult);

        // make sure the dir exists under the *correct* name
        if (openResult) {
            cleaner.tempNames << dir1.path()
                << dir2.path()
                << dir3.path()
                << dir4.path()
                << dir5.path()
                << dir6.path();

            QDir currentDir;
            QString fileName1 = currentDir.relativeFilePath(dir1.path());
            QString fileName2 = currentDir.relativeFilePath(dir2.path());
            QString fileName3 = currentDir.relativeFilePath(dir3.path());
            QString fileName4 = currentDir.relativeFilePath(dir4.path());
            QString fileName5 = currentDir.relativeFilePath(dir5.path());
            QString fileName6 = currentDir.relativeFilePath(dir6.path());

            QVERIFY(fileName1.startsWith(prefix));
            QVERIFY(fileName2.startsWith(prefix));
            QVERIFY(fileName5.startsWith("test-XXXXXX/" + prefix));
            QVERIFY(fileName6.startsWith("test-XXXXXX/" + prefix));

            if (!prefix.isEmpty()) {
                QVERIFY(fileName3.startsWith(prefix));
                QVERIFY(fileName4.startsWith(prefix));
            }
        }
    }

#ifdef Q_OS_WIN
    QTest::qWait(20);
#endif
    foreach (const QString &tempName, cleaner.tempNames)
        QVERIFY2(!QDir(tempName).exists(), qPrintable(tempName));

    cleaner.reset();
}

QTEST_MAIN(tst_QTemporaryDir)
#include "tst_qtemporarydir.moc"
