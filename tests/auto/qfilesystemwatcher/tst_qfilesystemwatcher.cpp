/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/
#include <QtTest/QtTest>

#include <QCoreApplication>

#include <QFileSystemWatcher>

#ifdef Q_OS_LINUX
# ifdef QT_NO_INOTIFY
#  include <linux/version.h>
# else
#  include <sys/inotify.h>
# endif
#endif

//TESTED_CLASS=
//TESTED_FILES=

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

    void cleanup();

    void QTBUG15255_deadlock();
private:
    QStringList do_force_engines;
    bool do_force_native;
};

tst_QFileSystemWatcher::tst_QFileSystemWatcher()
    : do_force_native(false)
{
#ifdef Q_OS_LINUX
    // the inotify implementation in the kernel is known to be buggy in certain versions of the linux kernel
    do_force_engines << "native";
    do_force_engines << "dnotify";

#ifdef QT_NO_INOTIFY
    if (LINUX_VERSION_CODE >= KERNEL_VERSION(2,6,13))
        do_force_engines << "inotify";
#else
    if (inotify_init() != -1)
        do_force_engines << "inotify";
#endif
#elif defined(Q_OS_WIN) || defined(Q_OS_DARWIN) || defined(Q_OS_FREEBSD) || defined(Q_OS_SYMBIAN)
    // we have native engines for win32, macosx and freebsd
    do_force_engines << "native";
#endif
}

void tst_QFileSystemWatcher::basicTest_data()
{
    QTest::addColumn<QString>("backend");
    foreach(QString engine, do_force_engines)
        QTest::newRow(engine.toLatin1().constData()) << engine;
    QTest::newRow("poller") << "poller";
}

void tst_QFileSystemWatcher::basicTest()
{
    QFETCH(QString, backend);
    qDebug() << "Testing" << backend << "engine";

    // create test file
    QFile testFile("testfile.txt");
    testFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);
    testFile.remove();
    QVERIFY(testFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    testFile.write(QByteArray("hello"));
    testFile.close();

    // set some file permissions
    testFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner);

    // create watcher, forcing it to use a specific backend
    QFileSystemWatcher watcher;
    watcher.setObjectName(QLatin1String("_qt_autotest_force_engine_") + backend);
    watcher.removePath(testFile.fileName());
    watcher.addPath(testFile.fileName());

    QSignalSpy changedSpy(&watcher, SIGNAL(fileChanged(const QString &)));
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

    // qDebug() << "waiting max 5 seconds for notification for file modification to trigger(1)";
    timer.start(5000);
    eventLoop.exec();

    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.at(0).count(), 1);

    QString fileName = changedSpy.at(0).at(0).toString();
    QCOMPARE(fileName, testFile.fileName());

    changedSpy.clear();

    // remove the watch and modify the file, should not get a signal from the watcher
    watcher.removePath(testFile.fileName());
    testFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    testFile.write(QByteArray("hello universe!"));
    testFile.close();

    // qDebug() << "waiting max 5 seconds for notification for file modification to trigger (2)";
    timer.start(5000);
    eventLoop.exec();

    QCOMPARE(changedSpy.count(), 0);

    // readd the file watch with a relative path
    watcher.addPath(testFile.fileName().prepend("./"));
    testFile.open(QIODevice::WriteOnly | QIODevice::Truncate);
    testFile.write(QByteArray("hello multiverse!"));
    testFile.close();

    timer.start(5000);
    eventLoop.exec();

    QVERIFY(changedSpy.count() > 0);

    watcher.removePath(testFile.fileName().prepend("./"));

    changedSpy.clear();

    // readd the file watch
    watcher.addPath(testFile.fileName());

    // change the permissions, should get a signal from the watcher
    testFile.setPermissions(QFile::ReadOwner);

    // qDebug() << "waiting max 5 seconds for notification for file permission modification to trigger(1)";
    timer.start(5000);
    eventLoop.exec();

    QCOMPARE(changedSpy.count(), 1);
    QCOMPARE(changedSpy.at(0).count(), 1);

    fileName = changedSpy.at(0).at(0).toString();
    QCOMPARE(fileName, testFile.fileName());

    changedSpy.clear();

    // remove the watch and modify file permissions, should not get a signal from the watcher
    watcher.removePath(testFile.fileName());
    testFile.setPermissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOther);

    // qDebug() << "waiting max 5 seconds for notification for file modification to trigger (2)";
    timer.start(5000);
    eventLoop.exec();

    QCOMPARE(changedSpy.count(), 0);

    // readd the file watch
    watcher.addPath(testFile.fileName());

    // remove the file, should get a signal from the watcher
    QVERIFY(testFile.remove());

    // qDebug() << "waiting max 5 seconds for notification for file removal to trigger";
    timer.start(5000);
    eventLoop.exec();

    QVERIFY(changedSpy.count() == 1 || changedSpy.count() == 2); // removing a file on some filesystems seems to deliver 2 notifications
    QCOMPARE(changedSpy.at(0).count(), 1);

    fileName = changedSpy.at(0).at(0).toString();
    QCOMPARE(fileName, testFile.fileName());

    changedSpy.clear();

    // recreate the file, we should not get any notification
    QVERIFY(testFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    testFile.write(QByteArray("hello"));
    testFile.close();

    // qDebug() << "waiting max 5 seconds for notification for file recreation to trigger";
    timer.start(5000);
    eventLoop.exec();

    QCOMPARE(changedSpy.count(), 0);

    QVERIFY(testFile.remove());
}

void tst_QFileSystemWatcher::watchDirectory()
{
    QFETCH(QString, backend);
    qDebug() << "Testing" << backend << "engine";

    QDir().mkdir("testDir");
    QDir testDir("testDir");

    QString testFileName = testDir.filePath("testFile.txt");
    QFile::remove(testFileName);

    QFileSystemWatcher watcher;
    watcher.setObjectName(QLatin1String("_qt_autotest_force_engine_") + backend);
    watcher.addPath(testDir.dirName());

    QSignalSpy changedSpy(&watcher, SIGNAL(directoryChanged(const QString &)));
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
    watcher.removePath(testDir.dirName());
    QVERIFY(testFile.open(QIODevice::WriteOnly | QIODevice::Truncate));
    testFile.close();

    // qDebug() << "waiting max 5 seconds for notification for file recreationg to trigger";
    timer.start(5000);
    eventLoop.exec();

    QCOMPARE(changedSpy.count(), 0);

    watcher.addPath(testDir.dirName());

    // remove the file again, should get a signal from the watcher
    QVERIFY(testFile.remove());

    timer.start(5000);
    eventLoop.exec();

    // remove the directory, should get a signal from the watcher
    QVERIFY(QDir().rmdir("testDir"));

    // qDebug() << "waiting max 5 seconds for notification for directory removal to trigger";
    timer.start(5000);
    eventLoop.exec();

#ifdef Q_OS_WINCE
    QEXPECT_FAIL("poller", "Directory does not get updated on file removal(See #137910)", Abort);
#endif
    QCOMPARE(changedSpy.count(), 2);
    QCOMPARE(changedSpy.at(0).count(), 1);
    QCOMPARE(changedSpy.at(1).count(), 1);

    fileName = changedSpy.at(0).at(0).toString();
    QCOMPARE(fileName, testDir.dirName());
    fileName = changedSpy.at(1).at(0).toString();
    QCOMPARE(fileName, testDir.dirName());

    changedSpy.clear();

    // recreate the file, we should not get any notification
    if (!QDir().mkdir("testDir"))
        QSKIP("Failed to recreate directory, skipping final test.", SkipSingle);

    // qDebug() << "waiting max 5 seconds for notification for dir recreation to trigger";
    timer.start(5000);
    eventLoop.exec();

    QCOMPARE(changedSpy.count(), 0);

    QVERIFY(QDir().rmdir("testDir"));
}

void tst_QFileSystemWatcher::addPath()
{
    QFileSystemWatcher watcher;
    QString home = QDir::homePath();
    watcher.addPath(home);
    QCOMPARE(watcher.directories().count(), 1);
    QCOMPARE(watcher.directories().first(), home);
    watcher.addPath(home);
    QCOMPARE(watcher.directories().count(), 1);

    // With empty string
    QTest::ignoreMessage(QtWarningMsg, "QFileSystemWatcher::addPath: path is empty");
    watcher.addPath(QString());
}

void tst_QFileSystemWatcher::removePath()
{
    QFileSystemWatcher watcher;
    QString home = QDir::homePath();
    watcher.addPath(home);
    watcher.removePath(home);
    QCOMPARE(watcher.directories().count(), 0);
    watcher.removePath(home);
    QCOMPARE(watcher.directories().count(), 0);

    // With empty string
    QTest::ignoreMessage(QtWarningMsg, "QFileSystemWatcher::removePath: path is empty");
    watcher.removePath(QString());
}

void tst_QFileSystemWatcher::addPaths()
{
    QFileSystemWatcher watcher;
    QStringList paths;
    paths << QDir::homePath() << QDir::currentPath();
    watcher.addPaths(paths);
    QCOMPARE(watcher.directories().count(), 2);

    // With empty list
    paths.clear();
    QTest::ignoreMessage(QtWarningMsg, "QFileSystemWatcher::addPaths: list is empty");
    watcher.addPaths(paths);
}

void tst_QFileSystemWatcher::removePaths()
{
    QFileSystemWatcher watcher;
    QStringList paths;
    paths << QDir::homePath() << QDir::currentPath();
    watcher.addPaths(paths);
    QCOMPARE(watcher.directories().count(), 2);
    watcher.removePaths(paths);
    QCOMPARE(watcher.directories().count(), 0);

    //With empty list
    paths.clear();
    QTest::ignoreMessage(QtWarningMsg, "QFileSystemWatcher::removePaths: list is empty");
    watcher.removePaths(paths);
}

#if 0
class SignalTest : public QObject {
    Q_OBJECT
    public slots:
        void fileSlot(const QString &file) { qDebug() << "file " << file;}
        void dirSlot(const QString &dir) { qDebug() << "dir" << dir;}
};
#endif

void tst_QFileSystemWatcher::watchFileAndItsDirectory()
{
    QFETCH(QString, backend);
    QDir().mkdir("testDir");
    QDir testDir("testDir");

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

    watcher.addPath(testDir.dirName());
    watcher.addPath(testFileName);

    /*
    SignalTest signalTest;
    QObject::connect(&watcher, SIGNAL(fileChanged(const QString &)), &signalTest, SLOT(fileSlot(const QString &)));
    QObject::connect(&watcher, SIGNAL(directoryChanged(const QString &)), &signalTest, SLOT(dirSlot(const QString &)));
    */

    QSignalSpy fileChangedSpy(&watcher, SIGNAL(fileChanged(const QString &)));
    QSignalSpy dirChangedSpy(&watcher, SIGNAL(directoryChanged(const QString &)));
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

    timer.start(3000);
    eventLoop.exec();
    QVERIFY(fileChangedSpy.count() > 0);
    //according to Qt 4 documentation:
    //void QFileSystemWatcher::directoryChanged ( const QString & path )   [signal]
    //This signal is emitted when the directory at a specified path, is modified
    //(e.g., when a file is added, -->modified<-- or deleted) or removed from disk.
    //Note that if there are several changes during a short period of time, some
    //of the changes might not emit this signal. However, the last change in the
    //sequence of changes will always generate this signal.
    //Symbian behaves as documented (and can't be filtered), but the other platforms don't
    //so test should not assert this
    QVERIFY(dirChangedSpy.count() < 2);

    if (backend == "dnotify")
        QSKIP("dnotify is broken, skipping the rest of the test.", SkipSingle);

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

    timer.start(3000);
    eventLoop.exec();
    QVERIFY(fileChangedSpy.count() > 0);
    QCOMPARE(dirChangedSpy.count(), 1);

    fileChangedSpy.clear();
    dirChangedSpy.clear();

    watcher.removePath(testFileName);
    QFile::remove(secondFileName);

    timer.start(3000);
    eventLoop.exec();
    QCOMPARE(fileChangedSpy.count(), 0);
    // polling watcher has generated separate events for content and time change
    // on Symbian emulator, so allow possibility of 2 events
    QVERIFY(dirChangedSpy.count() == 1 || dirChangedSpy.count() == 2);

    QVERIFY(QDir().rmdir("testDir"));
}

void tst_QFileSystemWatcher::cleanup()
{
    QDir testDir("testDir");
    QString testFileName = testDir.filePath("testFile.txt");
    QString secondFileName = testDir.filePath("testFile2.txt");
    QFile::remove(testFileName);
    QFile::remove(secondFileName);
    QDir().rmdir("testDir");
}

void tst_QFileSystemWatcher::nonExistingFile()
{
    // Don't crash...
    QFileSystemWatcher watcher;
    watcher.addPath("file_that_does_not_exist.txt");
    QVERIFY(true);
}

void tst_QFileSystemWatcher::removeFileAndUnWatch()
{
    static const char * const filename = "foo.txt";
    QFileSystemWatcher watcher;

    {
        QFile testFile(filename);
        testFile.open(QIODevice::WriteOnly);
        testFile.close();
    }
    watcher.addPath(filename);

    QFile::remove(filename);
    watcher.removePath(filename);

    {
        QFile testFile(filename);
        testFile.open(QIODevice::WriteOnly);
        testFile.close();
    }
    watcher.addPath(filename);
}

class SomeSingleton : public QObject
{
public:
    SomeSingleton() : mFsWatcher(new QFileSystemWatcher(this)) { mFsWatcher->addPath(QLatin1String("/usr/lib"));}
    void bla() const {}
    QFileSystemWatcher* mFsWatcher;
};

Q_GLOBAL_STATIC(SomeSingleton, someSingleton)

void tst_QFileSystemWatcher::QTBUG15255_deadlock()
{
    someSingleton()->bla();
    //the test must still finish
    QTest::qWait(30);
}


QTEST_MAIN(tst_QFileSystemWatcher)
#include "tst_qfilesystemwatcher.moc"
