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

#ifndef QT_NO_PROCESS
    void basicProcesses();

    void processes_data();
    void processes();

#ifndef Q_OS_WIN
    void undo();
#endif
    void initialValue();
#endif // QT_NO_PROCESS

private:
    QString helperBinary();
    QSystemSemaphore *existingLock;
};

tst_QSystemSemaphore::tst_QSystemSemaphore()
{
}

void tst_QSystemSemaphore::initTestCase()
{
  QVERIFY2(!helperBinary().isEmpty(), "Could not find helper binary");
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
    QVERIFY(sem.release());
    QVERIFY(sem.acquire());
    QVERIFY(sem.release());
    QVERIFY(sem.acquire());
    QVERIFY(sem.acquire());
    QVERIFY(sem.release());
    QVERIFY(sem.release());
    QCOMPARE(sem.error(), QSystemSemaphore::NoError);
    QCOMPARE(sem.errorString(), QString());
}

#ifndef QT_NO_PROCESS
void tst_QSystemSemaphore::basicProcesses()
{
    QSystemSemaphore sem("store", 0, QSystemSemaphore::Create);

    QProcess acquire;
    acquire.setProcessChannelMode(QProcess::ForwardedChannels);

    QProcess release;
    release.setProcessChannelMode(QProcess::ForwardedChannels);

    acquire.start(helperBinary(), QStringList("acquire"));
    QVERIFY2(acquire.waitForStarted(), "Could not start helper binary");
    acquire.waitForFinished(HELPERWAITTIME);
    QVERIFY(acquire.state() == QProcess::Running);
    acquire.kill();
    release.start(helperBinary(), QStringList("release"));
    QVERIFY2(release.waitForStarted(), "Could not start helper binary");
    acquire.waitForFinished(HELPERWAITTIME);
    release.waitForFinished(HELPERWAITTIME);
    QVERIFY(acquire.state() == QProcess::NotRunning);
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
    QSystemSemaphore sem("store", 1, QSystemSemaphore::Create);

    QFETCH(int, processes);
    QVector<QString> scripts(processes, "acquirerelease");

    QList<QProcess*> consumers;
    for (int i = 0; i < scripts.count(); ++i) {
        QProcess *p = new QProcess;
        p->setProcessChannelMode(QProcess::ForwardedChannels);
        consumers.append(p);
        p->start(helperBinary(), QStringList(scripts.at(i)));
    }

    while (!consumers.isEmpty()) {
        consumers.first()->waitForFinished();
        QCOMPARE(consumers.first()->exitStatus(), QProcess::NormalExit);
        QCOMPARE(consumers.first()->exitCode(), 0);
        delete consumers.takeFirst();
    }
}

// This test only checks a unix behavior.
#ifndef Q_OS_WIN
void tst_QSystemSemaphore::undo()
{
    QSystemSemaphore sem("store", 1, QSystemSemaphore::Create);

    QStringList acquireArguments = QStringList("acquire");
    QProcess acquire;
    acquire.setProcessChannelMode(QProcess::ForwardedChannels);
    acquire.start(helperBinary(), acquireArguments);
    QVERIFY2(acquire.waitForStarted(), "Could not start helper binary");
    acquire.waitForFinished(HELPERWAITTIME);
    QVERIFY(acquire.state()== QProcess::NotRunning);

    // At process exit the kernel should auto undo

    acquire.start(helperBinary(), acquireArguments);
    QVERIFY2(acquire.waitForStarted(), "Could not start helper binary");
    acquire.waitForFinished(HELPERWAITTIME);
    QVERIFY(acquire.state()== QProcess::NotRunning);
}
#endif

void tst_QSystemSemaphore::initialValue()
{
    QSystemSemaphore sem("store", 1, QSystemSemaphore::Create);

    QStringList acquireArguments = QStringList("acquire");
    QStringList releaseArguments = QStringList("release");
    QProcess acquire;
    acquire.setProcessChannelMode(QProcess::ForwardedChannels);

    QProcess release;
    release.setProcessChannelMode(QProcess::ForwardedChannels);

    acquire.start(helperBinary(), acquireArguments);
    QVERIFY2(acquire.waitForStarted(), "Could not start helper binary");
    acquire.waitForFinished(HELPERWAITTIME);
    QVERIFY(acquire.state()== QProcess::NotRunning);

    acquire.start(helperBinary(), acquireArguments << QLatin1String("2"));
    QVERIFY2(acquire.waitForStarted(), "Could not start helper binary");
    acquire.waitForFinished(HELPERWAITTIME);
    QVERIFY(acquire.state()== QProcess::Running);
    acquire.kill();

    release.start(helperBinary(), releaseArguments);
    QVERIFY2(release.waitForStarted(), "Could not start helper binary");
    acquire.waitForFinished(HELPERWAITTIME);
    release.waitForFinished(HELPERWAITTIME);
    QVERIFY(acquire.state()== QProcess::NotRunning);
}
#endif

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

