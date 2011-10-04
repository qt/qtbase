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
#include <qsystemsemaphore.h>
//TESTED_CLASS=
//TESTED_FILES=

#define EXISTING_SHARE "existing"
#ifdef Q_OS_WINCE
#define LACKEYLOC "."
#else
#define LACKEYLOC "../lackey"
#endif
#define LACKYWAITTIME 10000

class tst_QSystemSemaphore : public QObject
{
    Q_OBJECT

public:
    tst_QSystemSemaphore();
    virtual ~tst_QSystemSemaphore();

public Q_SLOTS:
    void init();
    void cleanup();

private slots:
    void key_data();
    void key();

    void basicacquire();
    void complexacquire();

    void basicProcesses();

    void processes_data();
    void processes();

#ifndef Q_OS_WIN
    void undo();
#endif
    void initialValue();

private:
    QSystemSemaphore *existingLock;

    QString makeFile(const QString &resource)
    {
        QFile memory(resource);
        if (!memory.open(QIODevice::ReadOnly)) {
            qDebug() << "error reading resource" << resource;
            return QString();
        }
        QTemporaryFile *file = new QTemporaryFile;
        file->open();
        file->write(memory.readAll());
        tempFiles.append(file);
        file->flush();
#ifdef Q_OS_WINCE
        // flush does not flush to disk on Windows CE. It flushes it into its application
        // cache. Thus we need to close the file to be able that other processes(lackey) can read it
        QString fileName = file->fileName();
        file->close();
        return fileName;
#endif
        return file->fileName();
    }

    QString acquire_js() { return makeFile(":/systemsemaphore_acquire.js"); }
    QString release_js() { return makeFile(":/systemsemaphore_release.js"); }
    QString acquirerelease_js() { return makeFile(":/systemsemaphore_acquirerelease.js"); }
    QList<QTemporaryFile*> tempFiles;
};

tst_QSystemSemaphore::tst_QSystemSemaphore()
{
    if (!QFile::exists(LACKEYLOC "/lackey"))
        qWarning() << "lackey executable doesn't exists!";
}

tst_QSystemSemaphore::~tst_QSystemSemaphore()
{
    qDeleteAll(tempFiles);
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

void tst_QSystemSemaphore::basicProcesses()
{
    QSystemSemaphore sem("store", 0, QSystemSemaphore::Create);

    QStringList acquireArguments = QStringList() << acquire_js();
    QStringList releaseArguments = QStringList() << release_js();
    QProcess acquire;
    acquire.setProcessChannelMode(QProcess::ForwardedChannels);

    QProcess release;
    release.setProcessChannelMode(QProcess::ForwardedChannels);

    acquire.start(LACKEYLOC "/lackey", acquireArguments);
    acquire.waitForFinished(LACKYWAITTIME);
    QVERIFY(acquire.state() == QProcess::Running);
    acquire.kill();
    release.start(LACKEYLOC "/lackey", releaseArguments);
    acquire.waitForFinished(LACKYWAITTIME);
    release.waitForFinished(LACKYWAITTIME);
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
    QStringList scripts;
    for (int i = 0; i < processes; ++i)
        scripts.append(acquirerelease_js());

    QList<QProcess*> consumers;
    for (int i = 0; i < scripts.count(); ++i) {
        QStringList arguments = QStringList() << scripts.at(i);
        QProcess *p = new QProcess;
        p->setProcessChannelMode(QProcess::ForwardedChannels);
        consumers.append(p);
#ifdef Q_OS_WINCE
        // We can't start the same executable twice on Windows CE.
        // Create a copy instead.
        QString lackeyCopy = QLatin1String(LACKEYLOC "/lackey");
        if (i > 0) {
            lackeyCopy.append(QString::number(i));
            lackeyCopy.append(QLatin1String(".exe"));
            if (!QFile::exists(lackeyCopy))
                QVERIFY(QFile::copy(LACKEYLOC "/lackey.exe", lackeyCopy));
        }
        p->start(lackeyCopy, arguments);
#else
        p->start(LACKEYLOC "/lackey", arguments);
#endif
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

    QStringList acquireArguments = QStringList() << acquire_js();
    QProcess acquire;
    acquire.setProcessChannelMode(QProcess::ForwardedChannels);
    acquire.start(LACKEYLOC "/lackey", acquireArguments);
    acquire.waitForFinished(LACKYWAITTIME);
    QVERIFY(acquire.state()== QProcess::NotRunning);

    // At process exit the kernel should auto undo

    acquire.start(LACKEYLOC "/lackey", acquireArguments);
    acquire.waitForFinished(LACKYWAITTIME);
    QVERIFY(acquire.state()== QProcess::NotRunning);
}
#endif

void tst_QSystemSemaphore::initialValue()
{
    QSystemSemaphore sem("store", 1, QSystemSemaphore::Create);

    QStringList acquireArguments = QStringList() << acquire_js();
    QStringList releaseArguments = QStringList() << release_js();
    QProcess acquire;
    acquire.setProcessChannelMode(QProcess::ForwardedChannels);

    QProcess release;
    release.setProcessChannelMode(QProcess::ForwardedChannels);

    acquire.start(LACKEYLOC "/lackey", acquireArguments);
    acquire.waitForFinished(LACKYWAITTIME);
    QVERIFY(acquire.state()== QProcess::NotRunning);

    acquire.start(LACKEYLOC "/lackey", acquireArguments << "2");
    acquire.waitForFinished(LACKYWAITTIME);
    QVERIFY(acquire.state()== QProcess::Running);
    acquire.kill();

    release.start(LACKEYLOC "/lackey", releaseArguments);
    acquire.waitForFinished(LACKYWAITTIME);
    release.waitForFinished(LACKYWAITTIME);
    QVERIFY(acquire.state()== QProcess::NotRunning);
}
QTEST_MAIN(tst_QSystemSemaphore)
#include "tst_qsystemsemaphore.moc"

