// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>
#if QT_CONFIG(process)
#include <QProcess>
#endif

#include <QtCore/QList>
#include <QtCore/QSystemSemaphore>
#include <QtCore/QTemporaryDir>

#define EXISTING_SHARE "existing"
#define HELPERWAITTIME 10000

class tst_QSystemSemaphore : public QObject
{
    Q_OBJECT

public:
    tst_QSystemSemaphore();

public Q_SLOTS:
    void init();
    void cleanup();

private slots:
    void key_data();
    void key();

    void basicacquire();
    void complexacquire();
    void release();

    void basicProcesses();

    void processes_data();
    void processes();

#if !defined(Q_OS_WIN) && !defined(QT_POSIX_IPC)
    void undo();
#endif
    void initialValue();

private:
    QSystemSemaphore *existingLock;

    const QString m_helperBinary;
};

tst_QSystemSemaphore::tst_QSystemSemaphore()
    : m_helperBinary("./acquirerelease_helper")
{
}

void tst_QSystemSemaphore::init()
{
    existingLock = new QSystemSemaphore(EXISTING_SHARE, 1, QSystemSemaphore::Create);
}

void tst_QSystemSemaphore::cleanup()
{
    delete existingLock;
}

void tst_QSystemSemaphore::key_data()
{
    QTest::addColumn<QString>("constructorKey");
    QTest::addColumn<QString>("setKey");

    QTest::newRow("null, null") << QString() << QString();
    QTest::newRow("null, one") << QString() << QString("one");
    QTest::newRow("one, two") << QString("one") << QString("two");
}

/*!
    Basic key testing
 */
void tst_QSystemSemaphore::key()
{
    QFETCH(QString, constructorKey);
    QFETCH(QString, setKey);

    QSystemSemaphore sem(constructorKey);
    QCOMPARE(sem.key(), constructorKey);
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QCOMPARE(sem.errorString(), QString());

    sem.setKey(setKey);
    QCOMPARE(sem.key(), setKey);
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QCOMPARE(sem.errorString(), QString());
}

void tst_QSystemSemaphore::basicacquire()
{
    QSystemSemaphore sem("QSystemSemaphore_basicacquire", 1, QSystemSemaphore::Create);
    QVERIFY(sem.acquire());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QVERIFY(sem.release());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QCOMPARE(sem.errorString(), QString());
}

void tst_QSystemSemaphore::complexacquire()
{
    QSystemSemaphore sem("QSystemSemaphore_complexacquire", 2, QSystemSemaphore::Create);
    QVERIFY(sem.acquire());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QVERIFY(sem.release());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QVERIFY(sem.acquire());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QVERIFY(sem.release());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QVERIFY(sem.acquire());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QVERIFY(sem.acquire());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QVERIFY(sem.release());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QVERIFY(sem.release());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QCOMPARE(sem.errorString(), QString());
}

void tst_QSystemSemaphore::release()
{
    QSystemSemaphore sem("QSystemSemaphore_release", 0, QSystemSemaphore::Create);
    QVERIFY(sem.release());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QVERIFY(sem.release());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QVERIFY(sem.acquire());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QVERIFY(sem.acquire());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QVERIFY(sem.release());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QVERIFY(sem.release());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QCOMPARE(sem.errorString(), QString());
}

void tst_QSystemSemaphore::basicProcesses()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support", SkipAll);
#else
    QSystemSemaphore sem("store", 0, QSystemSemaphore::Create);

    QProcess acquire;
    acquire.setProcessChannelMode(QProcess::ForwardedChannels);

    QProcess release;
    release.setProcessChannelMode(QProcess::ForwardedChannels);

    acquire.start(m_helperBinary, QStringList("acquire"));
    QVERIFY2(acquire.waitForStarted(), "Could not start helper binary");
    acquire.waitForFinished(HELPERWAITTIME);
    QCOMPARE(acquire.state(), QProcess::Running);
    acquire.kill();
    release.start(m_helperBinary, QStringList("release"));
    QVERIFY2(release.waitForStarted(), "Could not start helper binary");
    acquire.waitForFinished(HELPERWAITTIME);
    release.waitForFinished(HELPERWAITTIME);
    QCOMPARE(acquire.state(), QProcess::NotRunning);
#endif
}

void tst_QSystemSemaphore::processes_data()
{
    QTest::addColumn<int>("processes");
    for (int i = 0; i < 5; ++i) {
        QTest::addRow("1 process (%d)", i) << 1;
        QTest::addRow("3 process (%d)", i) << 3;
        QTest::addRow("10 process (%d)", i) << 10;
    }
}

void tst_QSystemSemaphore::processes()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support", SkipAll);
#else
    QSystemSemaphore sem("store", 1, QSystemSemaphore::Create);

    QFETCH(int, processes);
    QList<QString> scripts(processes, "acquirerelease");

    QList<QProcess*> consumers;
    for (int i = 0; i < scripts.size(); ++i) {
        QProcess *p = new QProcess;
        p->setProcessChannelMode(QProcess::ForwardedChannels);
        consumers.append(p);
        p->start(m_helperBinary, QStringList(scripts.at(i)));
    }

    while (!consumers.isEmpty()) {
        consumers.first()->waitForFinished();
        QCOMPARE(consumers.first()->exitStatus(), QProcess::NormalExit);
        QCOMPARE(consumers.first()->exitCode(), 0);
        delete consumers.takeFirst();
    }
#endif
}

// This test only checks a system v unix behavior.
#if !defined(Q_OS_WIN) && !defined(QT_POSIX_IPC)
void tst_QSystemSemaphore::undo()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support", SkipAll);
#else
    QSystemSemaphore sem("store", 1, QSystemSemaphore::Create);

    QStringList acquireArguments = QStringList("acquire");
    QProcess acquire;
    acquire.setProcessChannelMode(QProcess::ForwardedChannels);
    acquire.start(m_helperBinary, acquireArguments);
    QVERIFY2(acquire.waitForStarted(), "Could not start helper binary");
    acquire.waitForFinished(HELPERWAITTIME);
    QVERIFY(acquire.state()== QProcess::NotRunning);

    // At process exit the kernel should auto undo

    acquire.start(m_helperBinary, acquireArguments);
    QVERIFY2(acquire.waitForStarted(), "Could not start helper binary");
    acquire.waitForFinished(HELPERWAITTIME);
    QVERIFY(acquire.state()== QProcess::NotRunning);
#endif
}
#endif

void tst_QSystemSemaphore::initialValue()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support", SkipAll);
#else
    QSystemSemaphore sem("store", 1, QSystemSemaphore::Create);

    QStringList acquireArguments = QStringList("acquire");
    QStringList releaseArguments = QStringList("release");
    QProcess acquire;
    acquire.setProcessChannelMode(QProcess::ForwardedChannels);

    QProcess release;
    release.setProcessChannelMode(QProcess::ForwardedChannels);

    acquire.start(m_helperBinary, acquireArguments);
    QVERIFY2(acquire.waitForStarted(), "Could not start helper binary");
    acquire.waitForFinished(HELPERWAITTIME);
    QVERIFY(acquire.state()== QProcess::NotRunning);

    acquire.start(m_helperBinary, acquireArguments << QLatin1String("2"));
    QVERIFY2(acquire.waitForStarted(), "Could not start helper binary");
    acquire.waitForFinished(HELPERWAITTIME);
    QVERIFY(acquire.state()== QProcess::Running);
    acquire.kill();

    release.start(m_helperBinary, releaseArguments);
    QVERIFY2(release.waitForStarted(), "Could not start helper binary");
    acquire.waitForFinished(HELPERWAITTIME);
    release.waitForFinished(HELPERWAITTIME);
    QVERIFY(acquire.state()== QProcess::NotRunning);
#endif
}

QTEST_MAIN(tst_QSystemSemaphore)
#include "tst_qsystemsemaphore.moc"

