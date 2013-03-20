/****************************************************************************
**
** Copyright (C) 2013 David Faure <faure+bluesystems@kde.org>
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
#include <QtConcurrentRun>
#include <qlockfile.h>
#include <qtemporarydir.h>

class tst_QLockFile : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void lockUnlock();
    void lockOutOtherProcess();
    void lockOutOtherThread();
    void waitForLock_data();
    void waitForLock();
    void staleLockFromCrashedProcess_data();
    void staleLockFromCrashedProcess();
    void staleShortLockFromBusyProcess();
    void staleLongLockFromBusyProcess();
    void staleLockRace();
    void noPermissions();

public:
    QString m_helperApp;
    QTemporaryDir dir;
};

void tst_QLockFile::initTestCase()
{
#ifdef QT_NO_PROCESS
    QSKIP("This test requires QProcess support");
#else
    // chdir to our testdata path and execute helper apps relative to that.
    QString testdata_dir = QFileInfo(QFINDTESTDATA("qlockfiletesthelper")).absolutePath();
    QVERIFY2(QDir::setCurrent(testdata_dir), qPrintable("Could not chdir to " + testdata_dir));
    m_helperApp = "qlockfiletesthelper/qlockfile_test_helper";
#endif
}

void tst_QLockFile::lockUnlock()
{
    const QString fileName = dir.path() + "/lock1";
    QVERIFY(!QFile(fileName).exists());
    QLockFile lockFile(fileName);
    QVERIFY(lockFile.lock());
    QVERIFY(lockFile.isLocked());
    QCOMPARE(int(lockFile.error()), int(QLockFile::NoError));
    QVERIFY(QFile::exists(fileName));

    // Recursive locking is not allowed
    // (can't test lock() here, it would wait forever)
    QVERIFY(!lockFile.tryLock());
    QCOMPARE(int(lockFile.error()), int(QLockFile::LockFailedError));
    qint64 pid;
    QString hostname, appname;
    QVERIFY(lockFile.getLockInfo(&pid, &hostname, &appname));
    QCOMPARE(pid, QCoreApplication::applicationPid());
    QCOMPARE(appname, qAppName());
    QVERIFY(!lockFile.tryLock(200));
    QCOMPARE(int(lockFile.error()), int(QLockFile::LockFailedError));

    // Unlock deletes the lock file
    lockFile.unlock();
    QCOMPARE(int(lockFile.error()), int(QLockFile::NoError));
    QVERIFY(!lockFile.isLocked());
    QVERIFY(!QFile::exists(fileName));
}

void tst_QLockFile::lockOutOtherProcess()
{
    // Lock
    const QString fileName = dir.path() + "/lockOtherProcess";
    QLockFile lockFile(fileName);
    QVERIFY(lockFile.lock());

    // Other process can't acquire lock
    QProcess proc;
    proc.start(m_helperApp, QStringList() << fileName);
    QVERIFY2(proc.waitForStarted(), qPrintable(proc.errorString()));
    QVERIFY(proc.waitForFinished());
    QCOMPARE(proc.exitCode(), int(QLockFile::LockFailedError));

    // Unlock
    lockFile.unlock();
    QVERIFY(!QFile::exists(fileName));

    // Other process can now acquire lock
    int ret = QProcess::execute(m_helperApp, QStringList() << fileName);
    QCOMPARE(ret, int(QLockFile::NoError));
    // Lock doesn't survive process though (on clean exit)
    QVERIFY(!QFile::exists(fileName));
}

static QLockFile::LockError tryLockFromThread(const QString &fileName)
{
    QLockFile lockInThread(fileName);
    lockInThread.tryLock();
    return lockInThread.error();
}

void tst_QLockFile::lockOutOtherThread()
{
    const QString fileName = dir.path() + "/lockOtherThread";
    QLockFile lockFile(fileName);
    QVERIFY(lockFile.lock());

    // Other thread can't acquire lock
    QFuture<QLockFile::LockError> ret = QtConcurrent::run<QLockFile::LockError>(tryLockFromThread, fileName);
    QCOMPARE(ret.result(), QLockFile::LockFailedError);

    lockFile.unlock();

    // Now other thread can acquire lock
    QFuture<QLockFile::LockError> ret2 = QtConcurrent::run<QLockFile::LockError>(tryLockFromThread, fileName);
    QCOMPARE(ret2.result(), QLockFile::NoError);
}

static bool lockFromThread(const QString &fileName, int sleepMs, QSemaphore *semThreadReady, QSemaphore *semMainThreadDone)
{
    QLockFile lockFile(fileName);
    if (!lockFile.lock()) {
        qWarning() << "Locking failed" << lockFile.error();
        return false;
    }
    semThreadReady->release();
    QThread::msleep(sleepMs);
    semMainThreadDone->acquire();
    lockFile.unlock();
    return true;
}

void tst_QLockFile::waitForLock_data()
{
    QTest::addColumn<int>("testNumber");
    QTest::addColumn<int>("threadSleepMs");
    QTest::addColumn<bool>("releaseEarly");
    QTest::addColumn<int>("tryLockTimeout");
    QTest::addColumn<bool>("expectedResult");

    int tn = 0; // test number
    QTest::newRow("wait_forever_succeeds") << ++tn << 500 << true << -1   << true;
    QTest::newRow("wait_longer_succeeds")  << ++tn << 500 << true << 1000 << true;
    QTest::newRow("wait_zero_fails")       << ++tn << 500 << false << 0    << false;
    QTest::newRow("wait_not_enough_fails") << ++tn << 500 << false << 100  << false;
}

void tst_QLockFile::waitForLock()
{
    QFETCH(int, testNumber);
    QFETCH(int, threadSleepMs);
    QFETCH(bool, releaseEarly);
    QFETCH(int, tryLockTimeout);
    QFETCH(bool, expectedResult);

    const QString fileName = dir.path() + "/waitForLock" + QString::number(testNumber);
    QLockFile lockFile(fileName);
    QSemaphore semThreadReady, semMainThreadDone;
    // Lock file from a thread
    QFuture<bool> ret = QtConcurrent::run<bool>(lockFromThread, fileName, threadSleepMs, &semThreadReady, &semMainThreadDone);
    semThreadReady.acquire();

    if (releaseEarly) // let the thread release the lock after threadSleepMs
        semMainThreadDone.release();

    QCOMPARE(lockFile.tryLock(tryLockTimeout), expectedResult);
    if (expectedResult)
        QCOMPARE(int(lockFile.error()), int(QLockFile::NoError));
    else
        QCOMPARE(int(lockFile.error()), int(QLockFile::LockFailedError));

    if (!releaseEarly) // only let the thread release the lock now
        semMainThreadDone.release();

    QVERIFY(ret); // waits for the thread to finish
}

void tst_QLockFile::staleLockFromCrashedProcess_data()
{
    QTest::addColumn<int>("staleLockTime");

    // Test both use cases for QLockFile, should make no difference here.
    QTest::newRow("short") << 30000;
    QTest::newRow("long") << 0;
}

void tst_QLockFile::staleLockFromCrashedProcess()
{
    QFETCH(int, staleLockTime);
    const QString fileName = dir.path() + "/staleLockFromCrashedProcess";

    int ret = QProcess::execute(m_helperApp, QStringList() << fileName << "-crash");
    QCOMPARE(ret, int(QLockFile::NoError));
    QTRY_VERIFY(QFile::exists(fileName));

    QLockFile secondLock(fileName);
    secondLock.setStaleLockTime(staleLockTime);
    // tryLock detects and removes the stale lock (since the PID is dead)
#ifdef Q_OS_WIN
    // It can take a bit of time on Windows, though.
    QVERIFY(secondLock.tryLock(30000));
#else
    QVERIFY(secondLock.tryLock());
#endif
    QCOMPARE(int(secondLock.error()), int(QLockFile::NoError));
}

void tst_QLockFile::staleShortLockFromBusyProcess()
{
    const QString fileName = dir.path() + "/staleLockFromBusyProcess";

    QProcess proc;
    proc.start(m_helperApp, QStringList() << fileName << "-busy");
    QVERIFY2(proc.waitForStarted(), qPrintable(proc.errorString()));
    QTRY_VERIFY(QFile::exists(fileName));

    QLockFile secondLock(fileName);
    QVERIFY(!secondLock.tryLock()); // held by other process
    QCOMPARE(int(secondLock.error()), int(QLockFile::LockFailedError));
    qint64 pid;
    QString hostname, appname;
    QTRY_VERIFY(secondLock.getLockInfo(&pid, &hostname, &appname));
#ifdef Q_OS_UNIX
    QCOMPARE(pid, proc.pid());
#endif

    secondLock.setStaleLockTime(100);
    QTest::qSleep(100); // make the lock stale
    // We can't "steal" (delete+recreate) a lock file from a running process
    // until the file descriptor is closed.
    QVERIFY(!secondLock.tryLock());

    proc.waitForFinished();
    QVERIFY(secondLock.tryLock());
}

void tst_QLockFile::staleLongLockFromBusyProcess()
{
    const QString fileName = dir.path() + "/staleLockFromBusyProcess";

    QProcess proc;
    proc.start(m_helperApp, QStringList() << fileName << "-busy");
    QVERIFY2(proc.waitForStarted(), qPrintable(proc.errorString()));
    QTRY_VERIFY(QFile::exists(fileName));

    QLockFile secondLock(fileName);
    secondLock.setStaleLockTime(0);
    QVERIFY(!secondLock.tryLock(100)); // never stale
    QCOMPARE(int(secondLock.error()), int(QLockFile::LockFailedError));
    qint64 pid;
    QTRY_VERIFY(secondLock.getLockInfo(&pid, NULL, NULL));
    QVERIFY(pid > 0);

    // As long as the other process is running, we can't remove the lock file
    QVERIFY(!secondLock.removeStaleLockFile());

    proc.waitForFinished();
}

static QString tryStaleLockFromThread(const QString &fileName)
{
    QLockFile lockInThread(fileName + ".lock");
    lockInThread.setStaleLockTime(1000);
    if (!lockInThread.lock())
        return "Error locking: " + QString::number(lockInThread.error());

    // The concurrent use of the file below (write, read, delete) is protected by the lock file above.
    // (provided that it doesn't become stale due to this operation taking too long)
    QFile theFile(fileName);
    if (!theFile.open(QIODevice::WriteOnly))
        return "Couldn't open for write";
    theFile.write("Hello world");
    theFile.flush();
    theFile.close();
    QFile reader(fileName);
    if (!reader.open(QIODevice::ReadOnly))
        return "Couldn't open for read";
    const QByteArray read = reader.readAll();
    if (read != "Hello world")
        return "File didn't have the expected contents:" + read;
    reader.remove();
    return QString();
}

void tst_QLockFile::staleLockRace()
{
    // Multiple threads notice a stale lock at the same time
    // Only one thread should delete it, otherwise a race will ensue
    const QString fileName = dir.path() + "/sharedFile";
    const QString lockName = fileName + ".lock";
    int ret = QProcess::execute(m_helperApp, QStringList() << lockName << "-crash");
    QCOMPARE(ret, int(QLockFile::NoError));
    QTRY_VERIFY(QFile::exists(lockName));

    QThreadPool::globalInstance()->setMaxThreadCount(10);
    QFutureSynchronizer<QString> synchronizer;
    for (int i = 0; i < 8; ++i)
        synchronizer.addFuture(QtConcurrent::run<QString>(tryStaleLockFromThread, fileName));
    synchronizer.waitForFinished();
    foreach (const QFuture<QString> &future, synchronizer.futures())
        QVERIFY2(future.result().isEmpty(), qPrintable(future.result()));
}

void tst_QLockFile::noPermissions()
{
#ifdef Q_OS_WIN
    // A readonly directory still allows us to create files, on Windows.
    QSKIP("No permission testing on Windows");
#endif
    // Restore permissions so that the QTemporaryDir cleanup can happen
    class PermissionRestorer
    {
        QString m_path;
    public:
        PermissionRestorer(const QString& path)
            : m_path(path)
        {}

        ~PermissionRestorer()
        {
            QFile file(m_path);
            file.setPermissions(QFile::Permissions(QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner));
        }
    };

    const QString fileName = dir.path() + "/staleLock";
    QFile dirAsFile(dir.path()); // I have to use QFile to change a dir's permissions...
    QVERIFY2(dirAsFile.setPermissions(QFile::Permissions(0)), qPrintable(dir.path())); // no permissions
    PermissionRestorer permissionRestorer(dir.path());

    QLockFile lockFile(fileName);
    QVERIFY(!lockFile.lock());
    QCOMPARE(int(lockFile.error()), int(QLockFile::PermissionError));
}

QTEST_MAIN(tst_QLockFile)
#include "tst_qlockfile.moc"
