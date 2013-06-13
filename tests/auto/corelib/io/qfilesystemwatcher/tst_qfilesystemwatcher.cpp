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

#include <QCoreApplication>

#include <QTemporaryDir>
#include <QFileSystemWatcher>

/* All tests need to run in temporary directories not used
 * by the application to avoid non-deterministic failures on Windows
 * due to locked directories and left-overs from previous tests. */

class tst_QFileSystemWatcher : public QObject
{
    Q_OBJECT
public:
    tst_QFileSystemWatcher();

private slots:
    void basicTest_data();
    void basicTest();

    void watchDirectory_data() { basicTest_data(); }
    void watchDirectory();

    void addPath();
    void removePath();
    void addPaths();
    void removePaths();

    void watchFileAndItsDirectory_data() { basicTest_data(); }
    void watchFileAndItsDirectory();

    void nonExistingFile();

    void removeFileAndUnWatch();

    void destroyAfterQCoreApplication();

    void QTBUG2331();
    void QTBUG2331_data() { basicTest_data(); }

private:
    QString m_tempDirPattern;
};

tst_QFileSystemWatcher::tst_QFileSystemWatcher()
{
    m_tempDirPattern = QDir::tempPath();
    if (!m_tempDirPattern.endsWith(QLatin1Char('/')))
        m_tempDirPattern += QLatin1Char('/');
    m_tempDirPattern += QStringLiteral("tst_qfilesystemwatcherXXXXXX");
}

void tst_QFileSystemWatcher::basicTest_data()
{
    QTest::addColumn<QString>("backend");
    QTest::addColumn<QString>("testFileName");
    const QString testFile = QStringLiteral("testfile.txt");
    // QTBUG-31341: Test the UNICODE capabilities; ensure no QString::toLower()
    // is in the code path since that will lower case for example
    // LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE with context, whereas the Windows file
    // system will not.
    const QString specialCharacterFile =
        QString(QChar(ushort(0x130))) // LATIN_CAPITAL_LETTER_I_WITH_DOT_ABOVE
        + QChar(ushort(0x00DC)) // LATIN_CAPITAL_LETTER_U_WITH_DIAERESIS
        + QStringLiteral(".txt");

    QTest::newRow("native backend-testfile") << "native" << testFile;
    QTest::newRow("poller backend-testfile") << "poller" << testFile;
    QTest::newRow("native backend-specialchars") << "native" << specialCharacterFile;
}

void tst_QFileSystemWatcher::basicTest()
{
    QFETCH(QString, backend);
    QFETCH(QString, testFileName);

    // create test file
    QTemporaryDir temporaryDirectory(m_tempDirPattern);
    QVERIFY(temporaryDirectory.isValid());
    QFile testFile(temporaryDirectory.path() + QLatin1Char('/') + testFileName);
    QVERIFY(testFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    testFile.write(QByteArray("hello"));
    testFile.close();

    // set some file permissions
    testFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

    // create watcher, forcing it to use a specific backend
    QFileSystemWatcher watcher;
    watcher.setObjectName(QLatin1String("_qt_autotest_force_engine_") + backend);
    QVERIFY(watcher.addPath(testFile.fileName()));

    QSignalSpy changedSpy(&watcher, SIGNAL(fileChanged(QString)));
    QVERIFY(changedSpy.isValid());
    QEventLoop eventLoop;
    QTimer timer;
    connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));

    // modify the file, should get a signal from the watcher

    // resolution of the modification time is system dependent, but it's at most 1 second when using
    // the polling engine. I've heard rumors that FAT32 has a 2 second resolution. So, we have to
    // wait a bit before we can modify the file (hrmph)...
#ifndef Q_OS_WINCE
    QTest::qWait(2000);
#else
    // WinCE is always a little bit slower. Give it a little bit more time
    QTest::qWait(5000);
#endif

    testFile.open(QIODevice::WriteOnly | QIODevice::Append);
    testFile.write(QByteArray("world"));
    testFile.close();

    // waiting max 5 seconds for notification for file modification to trigger
    QTRY_COMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.at(0).count(), 1);

    QString fileName = changedSpy.at(0).at(0).toString();
    QCOMPARE(fileName, testFile.fileName());

    changedSpy.clear();

    // remove the watch and modify the file, should not get a signal from the watcher
    QVERIFY(watcher.removePath(testFile.fileName()));
    testFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    testFile.write(QByteArray("hello universe!"));
    testFile.close();

    // waiting max 5 seconds for notification for file modification to trigger
    timer.start(5000);
    eventLoop.exec();

    QCOMPARE(changedSpy.count(), 0);

    // readd the file watch with a relative path
    const QString relativeTestFileName = QDir::current().relativeFilePath(testFile.fileName());
    QVERIFY(!relativeTestFileName.isEmpty());
    QVERIFY(watcher.addPath(relativeTestFileName));
    testFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    testFile.write(QByteArray("hello multiverse!"));
    testFile.close();

    QTRY_VERIFY(changedSpy.count() > 0);

    QVERIFY(watcher.removePath(relativeTestFileName));

    changedSpy.clear();

    // readd the file watch
    QVERIFY(watcher.addPath(testFile.fileName()));

    // change the permissions, should get a signal from the watcher
    testFile.setPermissions(QFile::ReadOwner);

    // IN_ATTRIB doesn't work on QNX, so skip this test
#if !defined(Q_OS_QNX)

    // waiting max 5 seconds for notification for file permission modification to trigger
    QTRY_COMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.at(0).count(), 1);

    fileName = changedSpy.at(0).at(0).toString();
    QCOMPARE(fileName, testFile.fileName());

#endif

    changedSpy.clear();

    // remove the watch and modify file permissions, should not get a signal from the watcher
    QVERIFY(watcher.removePath(testFile.fileName()));
    testFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOther);

    // waiting max 5 seconds for notification for file modification to trigger
    timer.start(5000);
    eventLoop.exec();

    QCOMPARE(changedSpy.count(), 0);

    // readd the file watch
    QVERIFY(watcher.addPath(testFile.fileName()));

    // remove the file, should get a signal from the watcher
    QVERIFY(testFile.remove());

    // waiting max 5 seconds for notification for file removal to trigger
    // > 0 && < 3 because some platforms may emit two changes
    // XXX: which platforms? (QTBUG-23370)
    QTRY_VERIFY(changedSpy.count() > 0 && changedSpy.count() < 3);
    QCOMPARE(changedSpy.at(0).count(), 1);

    fileName = changedSpy.at(0).at(0).toString();
    QCOMPARE(fileName, testFile.fileName());

    changedSpy.clear();

    // recreate the file, we should not get any notification
    QVERIFY(testFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    testFile.write(QByteArray("hello"));
    testFile.close();

    // waiting max 5 seconds for notification for file recreation to trigger
    timer.start(5000);
    eventLoop.exec();

    QCOMPARE(changedSpy.count(), 0);

    QVERIFY(testFile.remove());
}

void tst_QFileSystemWatcher::watchDirectory()
{
    QFETCH(QString, backend);

    QTemporaryDir temporaryDirectory(m_tempDirPattern);
    QVERIFY(temporaryDirectory.isValid());

    QDir temporaryDir(temporaryDirectory.path());
    const QString testDirName = QStringLiteral("testDir");
    QVERIFY(temporaryDir.mkdir(testDirName));
    QDir testDir = temporaryDir;
    QVERIFY(testDir.cd(testDirName));

    QString testFileName = testDir.filePath("testFile.txt");
    QFile::remove(testFileName);

    QFileSystemWatcher watcher;
    watcher.setObjectName(QLatin1String("_qt_autotest_force_engine_") + backend);
    QVERIFY(watcher.addPath(testDir.absolutePath()));

    QSignalSpy changedSpy(&watcher, SIGNAL(directoryChanged(QString)));
    QVERIFY(changedSpy.isValid());
    QEventLoop eventLoop;
    QTimer timer;
    connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));

    // resolution of the modification time is system dependent, but it's at most 1 second when using
    // the polling engine. From what I know, FAT32 has a 2 second resolution. So we have to
    // wait before modifying the directory...
    QTest::qWait(2000);
    QFile testFile(testFileName);
    QString fileName;

    // remove the watch, should not get notification of a new file
    QVERIFY(watcher.removePath(testDir.absolutePath()));
    QVERIFY(testFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    testFile.close();

    // waiting max 5 seconds for notification for file recreationg to trigger
    timer.start(5000);
    eventLoop.exec();

    QCOMPARE(changedSpy.count(), 0);

    QVERIFY(watcher.addPath(testDir.absolutePath()));

    // remove the file again, should get a signal from the watcher
    QVERIFY(testFile.remove());

    timer.start(5000);
    eventLoop.exec();

    // remove the directory, should get a signal from the watcher
    QVERIFY(temporaryDir.rmdir(testDirName));

    // waiting max 5 seconds for notification for directory removal to trigger
#ifdef Q_OS_WINCE
    QEXPECT_FAIL("poller", "Directory does not get updated on file removal(See #137910)", Abort);
#endif
    QTRY_COMPARE(changedSpy.count(), 2);
    QCOMPARE(changedSpy.at(0).count(), 1);
    QCOMPARE(changedSpy.at(1).count(), 1);

    fileName = changedSpy.at(0).at(0).toString();
    QCOMPARE(fileName, testDir.absolutePath());
    fileName = changedSpy.at(1).at(0).toString();
    QCOMPARE(fileName, testDir.absolutePath());

    changedSpy.clear();

    // recreate the file, we should not get any notification
    if (!temporaryDir.mkdir(testDirName))
        QSKIP(qPrintable(QString::fromLatin1("Failed to recreate directory '%1' under '%2', skipping final test.").
                         arg(testDirName, temporaryDir.absolutePath())));

    // waiting max 5 seconds for notification for dir recreation to trigger
    timer.start(5000);
    eventLoop.exec();

    QCOMPARE(changedSpy.count(), 0);

    QVERIFY(temporaryDir.rmdir(testDirName));
}

void tst_QFileSystemWatcher::addPath()
{
    QFileSystemWatcher watcher;
    QString home = QDir::homePath();
    QVERIFY(watcher.addPath(home));
    QCOMPARE(watcher.directories().count(), 1);
    QCOMPARE(watcher.directories().first(), home);

    // second watch on an already-watched path should fail
    QVERIFY(!watcher.addPath(home));
    QCOMPARE(watcher.directories().count(), 1);

    // With empty string
    QTest::ignoreMessage(QtWarningMsg, "QFileSystemWatcher::addPath: path is empty");
    QVERIFY(watcher.addPath(QString()));
}

void tst_QFileSystemWatcher::removePath()
{
    QFileSystemWatcher watcher;
    QString home = QDir::homePath();
    QVERIFY(watcher.addPath(home));
    QVERIFY(watcher.removePath(home));
    QCOMPARE(watcher.directories().count(), 0);
    QVERIFY(!watcher.removePath(home));
    QCOMPARE(watcher.directories().count(), 0);

    // With empty string
    QTest::ignoreMessage(QtWarningMsg, "QFileSystemWatcher::removePath: path is empty");
    QVERIFY(watcher.removePath(QString()));
}

void tst_QFileSystemWatcher::addPaths()
{
    QFileSystemWatcher watcher;
    QStringList paths;
    paths << QDir::homePath() << QDir::currentPath();
    QCOMPARE(watcher.addPaths(paths), QStringList());
    QCOMPARE(watcher.directories().count(), 2);

    // With empty list
    paths.clear();
    QTest::ignoreMessage(QtWarningMsg, "QFileSystemWatcher::addPaths: list is empty");
    QCOMPARE(watcher.addPaths(paths), QStringList());
}

void tst_QFileSystemWatcher::removePaths()
{
    QFileSystemWatcher watcher;
    QStringList paths;
    paths << QDir::homePath() << QDir::currentPath();
    QCOMPARE(watcher.addPaths(paths), QStringList());
    QCOMPARE(watcher.directories().count(), 2);
    QCOMPARE(watcher.removePaths(paths), QStringList());
    QCOMPARE(watcher.directories().count(), 0);

    //With empty list
    paths.clear();
    QTest::ignoreMessage(QtWarningMsg, "QFileSystemWatcher::removePaths: list is empty");
    watcher.removePaths(paths);
}

void tst_QFileSystemWatcher::watchFileAndItsDirectory()
{
    QFETCH(QString, backend);

    QTemporaryDir temporaryDirectory(m_tempDirPattern);
    QVERIFY(temporaryDirectory.isValid());

    QDir temporaryDir(temporaryDirectory.path());
    const QString testDirName = QStringLiteral("testDir");
    QVERIFY(temporaryDir.mkdir(testDirName));
    QDir testDir = temporaryDir;
    QVERIFY(testDir.cd(testDirName));

    QString testFileName = testDir.filePath("testFile.txt");
    QString secondFileName = testDir.filePath("testFile2.txt");
    QFile::remove(secondFileName);

    QFile testFile(testFileName);
    testFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
    testFile.remove();

    QVERIFY(testFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    testFile.write(QByteArray("hello"));
    testFile.close();

    QFileSystemWatcher watcher;
    watcher.setObjectName(QLatin1String("_qt_autotest_force_engine_") + backend);

    QVERIFY(watcher.addPath(testDir.absolutePath()));
    QVERIFY(watcher.addPath(testFileName));

    QSignalSpy fileChangedSpy(&watcher, SIGNAL(fileChanged(QString)));
    QSignalSpy dirChangedSpy(&watcher, SIGNAL(directoryChanged(QString)));
    QVERIFY(fileChangedSpy.isValid());
    QVERIFY(dirChangedSpy.isValid());
    QEventLoop eventLoop;
    QTimer timer;
    connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));

    // resolution of the modification time is system dependent, but it's at most 1 second when using
    // the polling engine. From what I know, FAT32 has a 2 second resolution. So we have to
    // wait before modifying the directory...
    QTest::qWait(2000);

    QVERIFY(testFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    testFile.write(QByteArray("hello again"));
    testFile.close();

#ifdef Q_OS_MAC
    // wait again for the file's atime to be updated
    QTest::qWait(2000);
#endif

    QTRY_VERIFY(fileChangedSpy.count() > 0);

    //according to Qt 4 documentation:
    //void QFileSystemWatcher::directoryChanged ( const QString & path )   [signal]
    //This signal is emitted when the directory at a specified path, is modified
    //(e.g., when a file is added, -->modified<-- or deleted) or removed from disk.
    //Note that if there are several changes during a short period of time, some
    //of the changes might not emit this signal. However, the last change in the
    //sequence of changes will always generate this signal.
    QVERIFY(dirChangedSpy.count() < 2);

    fileChangedSpy.clear();
    dirChangedSpy.clear();
    QFile secondFile(secondFileName);
    secondFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    secondFile.write("Foo");
    secondFile.close();

    timer.start(3000);
    eventLoop.exec();
    QCOMPARE(fileChangedSpy.count(), 0);
#ifdef Q_OS_WINCE
    QEXPECT_FAIL("poller", "Directory does not get updated on file removal(See #137910)", Abort);
#endif
    QCOMPARE(dirChangedSpy.count(), 1);

    dirChangedSpy.clear();

    QFile::remove(testFileName);

    QTRY_VERIFY(fileChangedSpy.count() > 0);
    QCOMPARE(dirChangedSpy.count(), 1);

    fileChangedSpy.clear();
    dirChangedSpy.clear();

    // removing a deleted file should fail
    QVERIFY(!watcher.removePath(testFileName));
    QFile::remove(secondFileName);

    timer.start(3000);
    eventLoop.exec();
    QCOMPARE(fileChangedSpy.count(), 0);
    QCOMPARE(dirChangedSpy.count(), 1);

    QVERIFY(temporaryDir.rmdir(testDirName));
}

void tst_QFileSystemWatcher::nonExistingFile()
{
    // Don't crash...
    QFileSystemWatcher watcher;
    QVERIFY(!watcher.addPath("file_that_does_not_exist.txt"));

    // Test that the paths returned in error aren't messed with
    QCOMPARE(watcher.addPaths(QStringList() << "../..//./does-not-exist"),
                              QStringList() << "../..//./does-not-exist");

    // empty path is not actually a failure
    QCOMPARE(watcher.addPaths(QStringList() << QString()), QStringList());

    // empty path is not actually a failure
    QCOMPARE(watcher.removePaths(QStringList() << QString()), QStringList());
}

void tst_QFileSystemWatcher::removeFileAndUnWatch()
{
    QTemporaryDir temporaryDirectory(m_tempDirPattern);
    QVERIFY(temporaryDirectory.isValid());

    const QString filename = temporaryDirectory.path() + QStringLiteral("/foo.txt");

    QFileSystemWatcher watcher;

    {
        QFile testFile(filename);
        QVERIFY2(testFile.open(QIODevice::WriteOnly),
                 qPrintable(QString::fromLatin1("Cannot open %1 for writing: %2").arg(filename, testFile.errorString())));
        testFile.close();
    }
    QVERIFY(watcher.addPath(filename));

    QFile::remove(filename);
    /* There are potential race conditions here; the watcher thread might remove the file from its list
     * before the call to watcher.removePath(), which then fails. When that happens, the auto-signal
     * notification to remove the file from the watcher's main list will not be delivered before the next
     * event loop such that the call to watcher.addPath() fails since the file is still in the main list. */
    if (!watcher.removePath(filename))
        QSKIP("Skipping remaining test due to race condition.");

    {
        QFile testFile(filename);
        QVERIFY2(testFile.open(QIODevice::WriteOnly),
                 qPrintable(QString::fromLatin1("Cannot open %1 for writing: %2").arg(filename, testFile.errorString())));
        testFile.close();
    }
    QVERIFY(watcher.addPath(filename));
}

class SomeSingleton : public QObject
{
public:
    SomeSingleton() : mFsWatcher(new QFileSystemWatcher(this)) { mFsWatcher->addPath(QLatin1String("/usr/lib"));}
    void bla() const {}
    QFileSystemWatcher* mFsWatcher;
};

Q_GLOBAL_STATIC(SomeSingleton, someSingleton)

// This is a regression test for QTBUG-15255, where a deadlock occurred if a
// QFileSystemWatcher was destroyed after the QCoreApplication instance had
// been destroyed.  There are no explicit verification steps in this test --
// it is sufficient that the test terminates.
void tst_QFileSystemWatcher::destroyAfterQCoreApplication()
{
    someSingleton()->bla();
    QTest::qWait(30);
}

// regression test for QTBUG2331.
// essentially, on windows, directories were not unwatched after being deleted
// from the disk, causing all sorts of interesting problems.
void tst_QFileSystemWatcher::QTBUG2331()
{
    QFETCH(QString, backend);

    QTemporaryDir temporaryDirectory(m_tempDirPattern);
    QVERIFY(temporaryDirectory.isValid());
    QFileSystemWatcher watcher;
    watcher.setObjectName(QLatin1String("_qt_autotest_force_engine_") + backend);
    QVERIFY(watcher.addPath(temporaryDirectory.path()));

    // watch signal
    QSignalSpy changedSpy(&watcher, SIGNAL(directoryChanged(QString)));
    QVERIFY(changedSpy.isValid());

    // remove directory, we should get one change signal, and we should no longer
    // be watching the directory.
    QVERIFY(temporaryDirectory.remove());
    QTRY_COMPARE(changedSpy.count(), 1);
    QCOMPARE(watcher.directories(), QStringList());
}

QTEST_MAIN(tst_QFileSystemWatcher)
#include "tst_qfilesystemwatcher.moc"
