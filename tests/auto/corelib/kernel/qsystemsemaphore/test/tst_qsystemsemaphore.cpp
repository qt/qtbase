/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtTest/QtTest>
#include <QtCore/QSystemSemaphore>
#include <QtCore/QVector>
#include <QtCore/QTemporaryDir>

#define EXISTING_SHARE "existing"
#define HELPERWAITTIME 10000

class tst_QSystemSemaphore : public QObject
{
    Q_OBJECT

public:
    tst_QSystemSemaphore();

public Q_SLOTS:
    void initTestCase();
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
    static QString helperBinary();
    QSystemSemaphore *existingLock;

    const QString m_helperBinary;
};

tst_QSystemSemaphore::tst_QSystemSemaphore()
    : m_helperBinary(helperBinary())
{
}

void tst_QSystemSemaphore::initTestCase()
{
  QVERIFY2(!m_helperBinary.isEmpty(), "Could not find helper binary");
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
        QTest::newRow("1 process") << 1;
        QTest::newRow("3 process") << 3;
        QTest::newRow("10 process") << 10;
    }
}

void tst_QSystemSemaphore::processes()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support", SkipAll);
#else
    QSystemSemaphore sem("store", 1, QSystemSemaphore::Create);

    QFETCH(int, processes);
    QVector<QString> scripts(processes, "acquirerelease");

    QList<QProcess*> consumers;
    for (int i = 0; i < scripts.count(); ++i) {
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

QString tst_QSystemSemaphore::helperBinary()
{
    QString binary = QStringLiteral("helperbinary");
#ifdef Q_OS_WIN
    binary += QStringLiteral(".exe");
#endif
    return QFINDTESTDATA(binary);
}
QTEST_MAIN(tst_QSystemSemaphore)
#include "tst_qsystemsemaphore.moc"

