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
#include <QtTest/QtTest>

#include <QCoreApplication>

#include <QTemporaryDir>
#include <QFileSystemWatcher>
#include <QElapsedTimer>
#include <QTextStream>
#include <QDir>

/* All tests need to run in temporary directories not used
 * by the application to avoid non-deterministic failures on Windows
 * due to locked directories and left-overs from previous tests. */

class tst_QFileSystemWatcher : public QObject
{
    Q_OBJECT
public:
    tst_QFileSystemWatcher();

#ifndef QT_NO_FILESYSTEMWATCHER
private slots:
    void basicTest_data();
    void basicTest();

    void watchDirectory_data() { basicTest_data(); }
    void watchDirectory();

    void addPath();
    void removePath();
    void addPaths();
    void removePaths();
    void removePathsFilesInSameDirectory();

    void watchFileAndItsDirectory_data() { basicTest_data(); }
    void watchFileAndItsDirectory();

    void nonExistingFile();

    void removeFileAndUnWatch();

    void destroyAfterQCoreApplication();

    void QTBUG2331();
    void QTBUG2331_data() { basicTest_data(); }

    void signalsEmittedAfterFileMoved();

private:
    QString m_tempDirPattern;
#endif // QT_NO_FILESYSTEMWATCHER
};

tst_QFileSystemWatcher::tst_QFileSystemWatcher()
{
#ifndef QT_NO_FILESYSTEMWATCHER
    m_tempDirPattern = QDir::tempPath();
    if (!m_tempDirPattern.endsWith(QLatin1Char('/')))
        m_tempDirPattern += QLatin1Char('/');
    m_tempDirPattern += QStringLiteral("tst_qfilesystemwatcherXXXXXX");
#endif // QT_NO_FILESYSTEMWATCHER

#if defined(Q_OS_ANDROID) && !defined(Q_OS_ANDROID_NO_SDK)
    QDir::setCurrent(QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
#endif
}

#ifndef QT_NO_FILESYSTEMWATCHER
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

#if !defined(Q_OS_QNX) || !defined(QT_NO_INOTIFY)
    QTest::newRow("native backend-testfile") << "native" << testFile;
    QTest::newRow("native backend-specialchars") << "native" << specialCharacterFile;
#endif
    QTest::newRow("poller backend-testfile") << "poller" << testFile;
}

void tst_QFileSystemWatcher::basicTest()
{
    QFETCH(QString, backend);
    QFETCH(QString, testFileName);

    // create test file
    QTemporaryDir temporaryDirectory(m_tempDirPattern);
    QVERIFY2(temporaryDirectory.isValid(), qPrintable(temporaryDirectory.errorString()));
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

    QSignalSpy changedSpy(&watcher, &QFileSystemWatcher::fileChanged);
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
    QVERIFY2(temporaryDirectory.isValid(), qPrintable(temporaryDirectory.errorString()));

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

    QSignalSpy changedSpy(&watcher, &QFileSystemWatcher::directoryChanged);
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

    // flush pending signals (like the one from the rmdir above)
    timer.start(5000);
    eventLoop.exec();
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

// A signal spy that records the paths and times received for better diagnostics.
class FileSystemWatcherSpy : public QObject {
    Q_OBJECT
public:
    enum Mode {
        SpyOnDirectoryChanged,
        SpyOnFileChanged
    };

    explicit FileSystemWatcherSpy(QFileSystemWatcher *watcher, Mode mode)
    {
        connect(watcher, mode == SpyOnDirectoryChanged ?
                &QFileSystemWatcher::directoryChanged : &QFileSystemWatcher::fileChanged,
                this, &FileSystemWatcherSpy::spySlot);
        m_elapsedTimer.start();
    }

    int count() const { return m_entries.size(); }
    void clear()
    {
        m_entries.clear();
        m_elapsedTimer.restart();
    }

    QByteArray receivedFilesMessage() const
    {
        QString result;
        QTextStream str(&result);
        str << "At " << m_elapsedTimer.elapsed() << "ms, received "
            << count() << " changes: ";
        for (int i =0, e = m_entries.size(); i < e; ++i) {
            if (i)
                str << ", ";
            str << m_entries.at(i).timeStamp << "ms: " << QDir::toNativeSeparators(m_entries.at(i).path);
        }
        return result.toLocal8Bit();
    }

private slots:
    void spySlot(const QString &p) { m_entries.append(Entry(m_elapsedTimer.elapsed(), p)); }

private:
    struct Entry {
        Entry() : timeStamp(0) {}
        Entry(qint64 t, const QString &p) : timeStamp(t), path(p) {}

        qint64 timeStamp;
        QString path;
    };

    QElapsedTimer m_elapsedTimer;
    QList<Entry> m_entries;
};

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

void tst_QFileSystemWatcher::removePathsFilesInSameDirectory()
{
    // QTBUG-46449/Windows: Check the return values of removePaths().
    // When adding the 1st file, a thread is started to watch the temp path.
    // After adding and removing the 2nd file, the thread is still running and
    // success should be reported.
    QTemporaryFile file1(m_tempDirPattern);
    QTemporaryFile file2(m_tempDirPattern);
    QVERIFY2(file1.open(), qPrintable(file1.errorString()));
    QVERIFY2(file2.open(), qPrintable(file1.errorString()));
    const QString path1 = file1.fileName();
    const QString path2 = file2.fileName();
    file1.close();
    file2.close();
    QFileSystemWatcher watcher;
    QVERIFY(watcher.addPath(path1));
    QCOMPARE(watcher.files().size(), 1);
    QVERIFY(watcher.addPath(path2));
    QCOMPARE(watcher.files().size(), 2);
    QVERIFY(watcher.removePath(path1));
    QCOMPARE(watcher.files().size(), 1);
    QVERIFY(watcher.removePath(path2));
    QCOMPARE(watcher.files().size(), 0);
}

static QByteArray msgFileOperationFailed(const char *what, const QFile &f)
{
    return what + QByteArrayLiteral(" failed on \"")
        + QDir::toNativeSeparators(f.fileName()).toLocal8Bit()
        + QByteArrayLiteral("\": ") + f.errorString().toLocal8Bit();
}

void tst_QFileSystemWatcher::watchFileAndItsDirectory()
{
    QFETCH(QString, backend);

    QTemporaryDir temporaryDirectory(m_tempDirPattern);
    QVERIFY2(temporaryDirectory.isValid(), qPrintable(temporaryDirectory.errorString()));

    QDir temporaryDir(temporaryDirectory.path());
    const QString testDirName = QStringLiteral("testDir");
    QVERIFY(temporaryDir.mkdir(testDirName));
    QDir testDir = temporaryDir;
    QVERIFY(testDir.cd(testDirName));

    QString testFileName = testDir.filePath("testFile.txt");
    QString secondFileName = testDir.filePath("testFile2.txt");

    QFile testFile(testFileName);
    QVERIFY2(testFile.open(QIODevice::WriteOnly | QIODevice::Truncate), msgFileOperationFailed("open", testFile));
    QVERIFY2(testFile.write(QByteArrayLiteral("hello")) > 0, msgFileOperationFailed("write", testFile));
    testFile.close();

    QFileSystemWatcher watcher;
    watcher.setObjectName(QLatin1String("_qt_autotest_force_engine_") + backend);

    QVERIFY(watcher.addPath(testDir.absolutePath()));
    QVERIFY(watcher.addPath(testFileName));

    QSignalSpy fileChangedSpy(&watcher, &QFileSystemWatcher::fileChanged);
    FileSystemWatcherSpy dirChangedSpy(&watcher, FileSystemWatcherSpy::SpyOnDirectoryChanged);
    QVERIFY(fileChangedSpy.isValid());
    QEventLoop eventLoop;
    QTimer timer;
    connect(&timer, SIGNAL(timeout()), &eventLoop, SLOT(quit()));

    // resolution of the modification time is system dependent, but it's at most 1 second when using
    // the polling engine. From what I know, FAT32 has a 2 second resolution. So we have to
    // wait before modifying the directory...
    QTest::qWait(2000);

    QVERIFY2(testFile.open(QIODevice::WriteOnly | QIODevice::Truncate), msgFileOperationFailed("open", testFile));
    QVERIFY2(testFile.write(QByteArrayLiteral("hello again")), msgFileOperationFailed("write", testFile));
    testFile.close();

#ifdef Q_OS_MAC
    // wait again for the file's atime to be updated
    QTest::qWait(2000);
#endif

    QTRY_VERIFY(fileChangedSpy.count() > 0);
    QVERIFY2(dirChangedSpy.count() == 0, dirChangedSpy.receivedFilesMessage());

    fileChangedSpy.clear();
    QFile secondFile(secondFileName);
    QVERIFY2(secondFile.open(QIODevice::WriteOnly | QIODevice::Truncate), msgFileOperationFailed("open", secondFile));
    QVERIFY2(secondFile.write(QByteArrayLiteral("Foo")) > 0, msgFileOperationFailed("write", secondFile));
    secondFile.close();

    timer.start(3000);
    eventLoop.exec();
    int fileChangedSpyCount = fileChangedSpy.count();
#ifdef Q_OS_WIN
    if (fileChangedSpyCount != 0)
        QEXPECT_FAIL("", "See QTBUG-30943", Continue);
#endif
    QCOMPARE(fileChangedSpyCount, 0);
#ifdef Q_OS_WINCE
    QEXPECT_FAIL("poller", "Directory does not get updated on file removal(See #137910)", Abort);
#endif
    QCOMPARE(dirChangedSpy.count(), 1);

    dirChangedSpy.clear();

    QVERIFY(QFile::remove(testFileName));

    QTRY_VERIFY(fileChangedSpy.count() > 0);
    QTRY_COMPARE(dirChangedSpy.count(), 1);

    fileChangedSpy.clear();
    dirChangedSpy.clear();

    // removing a deleted file should fail
    QVERIFY(!watcher.removePath(testFileName));
    QVERIFY(QFile::remove(secondFileName));

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
    QVERIFY2(temporaryDirectory.isValid(), qPrintable(temporaryDirectory.errorString()));

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
    QVERIFY2(temporaryDirectory.isValid(), qPrintable(temporaryDirectory.errorString()));
    QFileSystemWatcher watcher;
    watcher.setObjectName(QLatin1String("_qt_autotest_force_engine_") + backend);
    QVERIFY(watcher.addPath(temporaryDirectory.path()));

    // watch signal
    QSignalSpy changedSpy(&watcher, &QFileSystemWatcher::directoryChanged);
    QVERIFY(changedSpy.isValid());

    // remove directory, we should get one change signal, and we should no longer
    // be watching the directory.
    QVERIFY(temporaryDirectory.remove());
    QTRY_COMPARE(changedSpy.count(), 1);
    QCOMPARE(watcher.directories(), QStringList());
}

class SignalReceiver : public QObject
{
    Q_OBJECT
public:
    SignalReceiver(const QDir &moveSrcDir,
                   const QString &moveDestination,
                   QFileSystemWatcher *watcher,
                   QObject *parent = 0)
        : QObject(parent),
          added(false),
          moveSrcDir(moveSrcDir),
          moveDestination(QDir(moveDestination)),
          watcher(watcher)
    {}

public slots:
    void fileChanged(const QString &path)
    {
        QFileInfo finfo(path);

        QCOMPARE(finfo.absolutePath(), moveSrcDir.absolutePath());

        if (!added) {
            foreach (const QFileInfo &fi, moveDestination.entryInfoList(QDir::Files | QDir::NoSymLinks))
                watcher->addPath(fi.absoluteFilePath());
            added = true;
        }
    }

private:
    bool added;
    QDir moveSrcDir;
    QDir moveDestination;
    QFileSystemWatcher *watcher;
};

// regression test for QTBUG-33211.
// using inotify backend if a file is moved and then added to the watcher
// before all the fileChanged signals are emitted the remaining signals are
// emitted with the destination path instead of the starting path
void tst_QFileSystemWatcher::signalsEmittedAfterFileMoved()
{
    const int fileCount = 10;
    QTemporaryDir temporaryDirectory(m_tempDirPattern);
    QVERIFY2(temporaryDirectory.isValid(), qPrintable(temporaryDirectory.errorString()));

    QDir testDir(temporaryDirectory.path());
    QVERIFY(testDir.mkdir("movehere"));
    QString movePath = testDir.filePath("movehere");

    for (int i = 0; i < fileCount; ++i) {
        QFile f(testDir.filePath(QString("test%1.txt").arg(i)));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(QByteArray("i am ") + QByteArray::number(i));
        f.close();
    }

    QFileSystemWatcher watcher;
    QVERIFY(watcher.addPath(testDir.path()));
    QVERIFY(watcher.addPath(movePath));

    // add files to watcher
    QFileInfoList files = testDir.entryInfoList(QDir::Files | QDir::NoSymLinks);
    QCOMPARE(files.size(), fileCount);
    foreach (const QFileInfo &finfo, files)
        QVERIFY(watcher.addPath(finfo.absoluteFilePath()));

    // create the signal receiver
    SignalReceiver signalReceiver(testDir, movePath, &watcher);
    connect(&watcher, SIGNAL(fileChanged(QString)), &signalReceiver, SLOT(fileChanged(QString)));

    // watch signals
    FileSystemWatcherSpy changedSpy(&watcher, FileSystemWatcherSpy::SpyOnFileChanged);
    QCOMPARE(changedSpy.count(), 0);

    // move files to second directory
    foreach (const QFileInfo &finfo, files)
        QVERIFY(testDir.rename(finfo.fileName(), QString("movehere/%2").arg(finfo.fileName())));

    QCoreApplication::processEvents();
    QVERIFY2(changedSpy.count() <= fileCount, changedSpy.receivedFilesMessage());
    QTRY_COMPARE(changedSpy.count(), fileCount);
}
#endif // QT_NO_FILESYSTEMWATCHER

QTEST_MAIN(tst_QFileSystemWatcher)
#include "tst_qfilesystemwatcher.moc"
