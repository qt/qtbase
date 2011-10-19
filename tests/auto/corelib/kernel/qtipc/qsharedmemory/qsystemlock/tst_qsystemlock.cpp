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
#include <qsystemlock.h>

//TESTED_CLASS=
//TESTED_FILES=

#define EXISTING_SHARE "existing"

class tst_QSystemLock : public QObject
{
    Q_OBJECT

public:
    tst_QSystemLock();
    virtual ~tst_QSystemLock();

public Q_SLOTS:
    void init();
    void cleanup();

private slots:
    void key_data();
    void key();

    void basicLock();
    void complexLock();
    void lockModes();
    void sucessive();
    void processes_data();
    void processes();

private:
    QSystemLock *existingLock;

};

tst_QSystemLock::tst_QSystemLock()
{
}

tst_QSystemLock::~tst_QSystemLock()
{
}

void tst_QSystemLock::init()
{
    existingLock = new QSystemLock(EXISTING_SHARE);
}

void tst_QSystemLock::cleanup()
{
    delete existingLock;
}

void tst_QSystemLock::key_data()
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
void tst_QSystemLock::key()
{
    QFETCH(QString, constructorKey);
    QFETCH(QString, setKey);

    QSystemLock sl(constructorKey);
    QCOMPARE(sl.key(), constructorKey);
    sl.setKey(setKey);
    QCOMPARE(sl.key(), setKey);
}

void tst_QSystemLock::basicLock()
{
    QSystemLock lock("foo");
    QVERIFY(lock.lock());
    QVERIFY(lock.unlock());
}

void tst_QSystemLock::complexLock()
{
    QSystemLock lock("foo");
    QVERIFY(lock.lock(QSystemLock::ReadOnly));
    QVERIFY(lock.unlock());

    QVERIFY(lock.lock(QSystemLock::ReadWrite));
    QVERIFY(lock.unlock());

    QVERIFY(lock.lock(QSystemLock::ReadOnly));
    QVERIFY(lock.lock(QSystemLock::ReadOnly));
    QVERIFY(lock.unlock());
    QVERIFY(lock.unlock());
}

void tst_QSystemLock::lockModes()
{
    QSystemLock reader1("library");
    QSystemLock reader2("library");

    QSystemLock librarian("library");
    QVERIFY(reader1.lock(QSystemLock::ReadOnly));
    QVERIFY(reader2.lock(QSystemLock::ReadOnly));
    QVERIFY(reader1.unlock());
    QVERIFY(reader2.unlock());
    QVERIFY(librarian.lock(QSystemLock::ReadWrite));
    QVERIFY(librarian.unlock());
}

void tst_QSystemLock::sucessive()
{
    QSystemLock lock("library");
    QVERIFY(lock.lock(QSystemLock::ReadOnly));
    QVERIFY(lock.lock(QSystemLock::ReadOnly));
    QVERIFY(lock.lock(QSystemLock::ReadOnly));
    QVERIFY(lock.lock(QSystemLock::ReadOnly));
    QVERIFY(lock.lock(QSystemLock::ReadOnly));
    QVERIFY(lock.unlock());
    QVERIFY(lock.unlock());
    QVERIFY(lock.unlock());
    QVERIFY(lock.unlock());
    QVERIFY(lock.unlock());
    QVERIFY(!lock.unlock());
}

void tst_QSystemLock::processes_data()
{
    QTest::addColumn<int>("readOnly");
    QTest::addColumn<int>("readWrite");
    for (int i = 0; i < 5; ++i) {
        QTest::newRow("1/0 process") << 1 << 0;
        QTest::newRow("0/1 process") << 0 << 1;
    	QTest::newRow("0/4 process") << 0 << 4;
        QTest::newRow("1/1 process") << 1 << 1;
        QTest::newRow("10/1 process") << 10 << 1;
        QTest::newRow("1/10 process") << 1 << 10;
        QTest::newRow("10/10 processes") << 10 << 10;
    }
}

/*!
    Create external processes
 */
void tst_QSystemLock::processes()
{
    QSKIP("This test takes about 15 minutes and needs to be trimmed down before we can re-enable it");
    QFETCH(int, readOnly);
    QFETCH(int, readWrite);

    QStringList scripts;
    for (int i = 0; i < readOnly; ++i)
        scripts.append(QFileInfo(SRCDIR "/../lackey/scripts/systemlock_read.js").absoluteFilePath() );
    for (int i = 0; i < readWrite; ++i)
        scripts.append(QFileInfo(SRCDIR "/../lackey/scripts/systemlock_readwrite.js").absoluteFilePath());

    QList<QProcess*> consumers;
    unsigned int failedProcesses = 0;
    for (int i = 0; i < scripts.count(); ++i) {

        QStringList arguments = QStringList() << scripts.at(i);
        QProcess *p = new QProcess;
        p->setProcessChannelMode(QProcess::ForwardedChannels);

        p->start("../lackey/lackey", arguments);
        // test, if the process could be started.

        if (p->waitForStarted(2000))
            consumers.append(p);
        else
            ++failedProcesses;
    }

    while (!consumers.isEmpty()) {
        consumers.first()->waitForFinished(3000);
        consumers.first()->kill();
        QCOMPARE(consumers.first()->exitStatus(), QProcess::NormalExit);
        QCOMPARE(consumers.first()->exitCode(), 0);
        delete consumers.takeFirst();
    }
    QCOMPARE(failedProcesses, (unsigned int)(0));
}

QTEST_MAIN(tst_QSystemLock)
#include "tst_qsystemlock.moc"

