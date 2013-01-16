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
#include <quuid.h>

class tst_QUuid : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void fromChar();
    void toString();
    void fromString();
    void toByteArray();
    void fromByteArray();
    void toRfc4122();
    void fromRfc4122();
    void createUuidV3OrV5();
    void check_QDataStream();
    void isNull();
    void equal();
    void notEqual();

    // Only in Qt > 3.2.x
    void generate();
    void less();
    void more();
    void variants();
    void versions();

    void threadUniqueness();
#ifndef QT_NO_PROCESS
    void processUniqueness();
#endif

    void hash();

    void qvariant();
    void qvariant_conversion();

public:
    // Variables
    QUuid uuidNS;
    QUuid uuidA;
    QUuid uuidB;
    QUuid uuidC;
    QUuid uuidD;
};

void tst_QUuid::initTestCase()
{
    //It's NameSpace_DNS in RFC4122
    //"{6ba7b810-9dad-11d1-80b4-00c04fd430c8}";
    uuidNS = QUuid(0x6ba7b810, 0x9dad, 0x11d1, 0x80, 0xb4, 0x00, 0xc0, 0x4f, 0xd4, 0x30, 0xc8);

    //"{fc69b59e-cc34-4436-a43c-ee95d128b8c5}";
    uuidA = QUuid(0xfc69b59e, 0xcc34, 0x4436, 0xa4, 0x3c, 0xee, 0x95, 0xd1, 0x28, 0xb8, 0xc5);

    //"{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}";
    uuidB = QUuid(0x1ab6e93a, 0xb1cb, 0x4a87, 0xba, 0x47, 0xec, 0x7e, 0x99, 0x03, 0x9a, 0x7b);

    // chdir to the directory containing our testdata, then refer to it with relative paths
    QString testdata_dir = QFileInfo(QFINDTESTDATA("testProcessUniqueness")).absolutePath();
    QVERIFY2(QDir::setCurrent(testdata_dir), qPrintable("Could not chdir to " + testdata_dir));

    //"{3d813cbb-47fb-32ba-91df-831e1593ac29}"; http://www.rfc-editor.org/errata_search.php?rfc=4122&eid=1352
    uuidC = QUuid(0x3d813cbb, 0x47fb, 0x32ba, 0x91, 0xdf, 0x83, 0x1e, 0x15, 0x93, 0xac, 0x29);

    //"{21f7f8de-8051-5b89-8680-0195ef798b6a}";
    uuidD = QUuid(0x21f7f8de, 0x8051, 0x5b89, 0x86, 0x80, 0x01, 0x95, 0xef, 0x79, 0x8b, 0x6a);
}

void tst_QUuid::fromChar()
{
    QCOMPARE(uuidA, QUuid("{fc69b59e-cc34-4436-a43c-ee95d128b8c5}"));
    QCOMPARE(uuidA, QUuid("fc69b59e-cc34-4436-a43c-ee95d128b8c5}"));
    QCOMPARE(uuidA, QUuid("{fc69b59e-cc34-4436-a43c-ee95d128b8c5"));
    QCOMPARE(uuidA, QUuid("fc69b59e-cc34-4436-a43c-ee95d128b8c5"));
    QCOMPARE(QUuid(), QUuid("{fc69b59e-cc34-4436-a43c-ee95d128b8c"));
    QCOMPARE(QUuid(), QUuid("{fc69b59e-cc34"));
    QCOMPARE(QUuid(), QUuid("fc69b59e-cc34-"));
    QCOMPARE(QUuid(), QUuid("fc69b59e-cc34"));
    QCOMPARE(QUuid(), QUuid("cc34"));
    QCOMPARE(QUuid(), QUuid(NULL));

    QCOMPARE(uuidB, QUuid(QString("{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}")));
}

void tst_QUuid::toString()
{
    QCOMPARE(uuidA.toString(), QString("{fc69b59e-cc34-4436-a43c-ee95d128b8c5}"));

    QCOMPARE(uuidB.toString(), QString("{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}"));
}

void tst_QUuid::fromString()
{
    QCOMPARE(uuidA, QUuid(QString("{fc69b59e-cc34-4436-a43c-ee95d128b8c5}")));
    QCOMPARE(uuidA, QUuid(QString("fc69b59e-cc34-4436-a43c-ee95d128b8c5}")));
    QCOMPARE(uuidA, QUuid(QString("{fc69b59e-cc34-4436-a43c-ee95d128b8c5")));
    QCOMPARE(uuidA, QUuid(QString("fc69b59e-cc34-4436-a43c-ee95d128b8c5")));
    QCOMPARE(QUuid(), QUuid(QString("{fc69b59e-cc34-4436-a43c-ee95d128b8c")));

    QCOMPARE(uuidB, QUuid(QString("{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}")));
}

void tst_QUuid::toByteArray()
{
    QCOMPARE(uuidA.toByteArray(), QByteArray("{fc69b59e-cc34-4436-a43c-ee95d128b8c5}"));

    QCOMPARE(uuidB.toByteArray(), QByteArray("{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}"));
}

void tst_QUuid::fromByteArray()
{
    QCOMPARE(uuidA, QUuid(QByteArray("{fc69b59e-cc34-4436-a43c-ee95d128b8c5}")));
    QCOMPARE(uuidA, QUuid(QByteArray("fc69b59e-cc34-4436-a43c-ee95d128b8c5}")));
    QCOMPARE(uuidA, QUuid(QByteArray("{fc69b59e-cc34-4436-a43c-ee95d128b8c5")));
    QCOMPARE(uuidA, QUuid(QByteArray("fc69b59e-cc34-4436-a43c-ee95d128b8c5")));
    QCOMPARE(QUuid(), QUuid(QByteArray("{fc69b59e-cc34-4436-a43c-ee95d128b8c")));

    QCOMPARE(uuidB, QUuid(QByteArray("{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}")));
}

void tst_QUuid::toRfc4122()
{
    QCOMPARE(uuidA.toRfc4122(), QByteArray::fromHex("fc69b59ecc344436a43cee95d128b8c5"));

    QCOMPARE(uuidB.toRfc4122(), QByteArray::fromHex("1ab6e93ab1cb4a87ba47ec7e99039a7b"));
}

void tst_QUuid::fromRfc4122()
{
    QCOMPARE(uuidA, QUuid::fromRfc4122(QByteArray::fromHex("fc69b59ecc344436a43cee95d128b8c5")));

    QCOMPARE(uuidB, QUuid::fromRfc4122(QByteArray::fromHex("1ab6e93ab1cb4a87ba47ec7e99039a7b")));
}

void tst_QUuid::createUuidV3OrV5()
{
    //"www.widgets.com" is also from RFC4122
    QCOMPARE(uuidC, QUuid::createUuidV3(uuidNS, QByteArray("www.widgets.com")));
    QCOMPARE(uuidC, QUuid::createUuidV3(uuidNS, QString("www.widgets.com")));

    QCOMPARE(uuidD, QUuid::createUuidV5(uuidNS, QByteArray("www.widgets.com")));
    QCOMPARE(uuidD, QUuid::createUuidV5(uuidNS, QString("www.widgets.com")));
}

void tst_QUuid::check_QDataStream()
{
    QUuid tmp;
    QByteArray ar;
    {
        QDataStream out(&ar,QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::BigEndian);
        out << uuidA;
    }
    {
        QDataStream in(&ar,QIODevice::ReadOnly);
        in.setByteOrder(QDataStream::BigEndian);
        in >> tmp;
        QCOMPARE(uuidA, tmp);
    }
    {
        QDataStream out(&ar,QIODevice::WriteOnly);
        out.setByteOrder(QDataStream::LittleEndian);
        out << uuidA;
    }
    {
        QDataStream in(&ar,QIODevice::ReadOnly);
        in.setByteOrder(QDataStream::LittleEndian);
        in >> tmp;
        QCOMPARE(uuidA, tmp);
    }
}

void tst_QUuid::isNull()
{
    QVERIFY( !uuidA.isNull() );

    QUuid should_be_null_uuid;
    QVERIFY( should_be_null_uuid.isNull() );
}


void tst_QUuid::equal()
{
    QVERIFY( !(uuidA == uuidB) );

    QUuid copy(uuidA);
    QVERIFY(uuidA == copy);

    QUuid assigned;
    assigned = uuidA;
    QVERIFY(uuidA == assigned);
}


void tst_QUuid::notEqual()
{
    QVERIFY( uuidA != uuidB );
}


void tst_QUuid::generate()
{
    QUuid shouldnt_be_null_uuidA = QUuid::createUuid();
    QUuid shouldnt_be_null_uuidB = QUuid::createUuid();
    QVERIFY( !shouldnt_be_null_uuidA.isNull() );
    QVERIFY( !shouldnt_be_null_uuidB.isNull() );
    QVERIFY( shouldnt_be_null_uuidA != shouldnt_be_null_uuidB );
}


void tst_QUuid::less()
{
    QVERIFY( !(uuidA < uuidB) );

    QUuid null_uuid;
    QVERIFY(null_uuid < uuidA); // Null uuid is always less than a valid one
}


void tst_QUuid::more()
{
    QVERIFY( uuidA > uuidB );

    QUuid null_uuid;
    QVERIFY( !(null_uuid > uuidA) ); // Null uuid is always less than a valid one
}


void tst_QUuid::variants()
{
    QVERIFY( uuidA.variant() == QUuid::DCE );
    QVERIFY( uuidB.variant() == QUuid::DCE );

    QUuid NCS = "{3a2f883c-4000-000d-0000-00fb40000000}";
    QVERIFY( NCS.variant() == QUuid::NCS );
}


void tst_QUuid::versions()
{
    QVERIFY( uuidA.version() == QUuid::Random );
    QVERIFY( uuidB.version() == QUuid::Random );

    QUuid DCE_time= "{406c45a0-3b7e-11d0-80a3-0000c08810a7}";
    QVERIFY( DCE_time.version() == QUuid::Time );

    QUuid NCS = "{3a2f883c-4000-000d-0000-00fb40000000}";
    QVERIFY( NCS.version() == QUuid::VerUnknown );
}

class UuidThread : public QThread
{
public:
    QUuid uuid;

    void run()
    {
        uuid = QUuid::createUuid();
    }
};

void tst_QUuid::threadUniqueness()
{
    QVector<UuidThread *> threads(qMax(2, QThread::idealThreadCount()));
    for (int i = 0; i < threads.count(); ++i)
        threads[i] = new UuidThread;
    for (int i = 0; i < threads.count(); ++i)
        threads[i]->start();
    for (int i = 0; i < threads.count(); ++i)
        QVERIFY(threads[i]->wait(1000));
    for (int i = 1; i < threads.count(); ++i)
        QVERIFY(threads[0]->uuid != threads[i]->uuid);
    qDeleteAll(threads);
}

#ifndef QT_NO_PROCESS
void tst_QUuid::processUniqueness()
{
    QProcess process;
    QString processOneOutput;
    QString processTwoOutput;

    // Start it once
#ifdef Q_OS_MAC
    process.start("testProcessUniqueness/testProcessUniqueness.app");
#else
    process.start("testProcessUniqueness/testProcessUniqueness");
#endif
    QVERIFY(process.waitForFinished());
    processOneOutput = process.readAllStandardOutput();

    // Start it twice
#ifdef Q_OS_MAC
    process.start("testProcessUniqueness/testProcessUniqueness.app");
#else
    process.start("testProcessUniqueness/testProcessUniqueness");
#endif
    QVERIFY(process.waitForFinished());
    processTwoOutput = process.readAllStandardOutput();

    // They should be *different*!
    QVERIFY(processOneOutput != processTwoOutput);
}
#endif

void tst_QUuid::hash()
{
    uint h = qHash(uuidA);
    QCOMPARE(qHash(uuidA), h);
    QCOMPARE(qHash(QUuid(uuidA.toString())), h);
}

void tst_QUuid::qvariant()
{
    QUuid uuid = QUuid::createUuid();
    QVariant v = QVariant::fromValue(uuid);
    QVERIFY(!v.isNull());
    QCOMPARE(v.type(), QVariant::Uuid);

    QUuid uuid2 = v.value<QUuid>();
    QVERIFY(!uuid2.isNull());
    QCOMPARE(uuid, uuid2);
}

void tst_QUuid::qvariant_conversion()
{
    QUuid uuid = QUuid::createUuid();
    QVariant v = QVariant::fromValue(uuid);

    QVERIFY(v.canConvert<QString>());
    QCOMPARE(v.toString(), uuid.toString());
    QCOMPARE(v.value<QString>(), uuid.toString());
    QVERIFY(!v.canConvert<int>());
    QVERIFY(!v.canConvert<QStringList>());

    // try reverse conversion QString -> QUuid
    QVariant sv = QVariant::fromValue(uuid.toString());
    QCOMPARE(sv.type(), QVariant::String);
    QVERIFY(sv.canConvert<QUuid>());
    QCOMPARE(sv.value<QUuid>(), uuid);
}

QTEST_MAIN(tst_QUuid)
#include "tst_quuid.moc"
