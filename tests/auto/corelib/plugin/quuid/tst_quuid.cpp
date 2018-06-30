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

#include <qcoreapplication.h>
#include <quuid.h>

class tst_QUuid : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();

    void fromChar();
    void toString();
    void fromString_data();
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
    void cpp11();

    // Only in Qt > 3.2.x
    void generate();
    void less();
    void more();
    void variants();
    void versions();

    void threadUniqueness();
    void processUniqueness();

    void hash();

    void qvariant();
    void qvariant_conversion();

    void darwinTypes();

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

#if QT_CONFIG(process)
    // chdir to the directory containing our testdata, then refer to it with relative paths
#ifdef Q_OS_ANDROID
    QString testdata_dir = QCoreApplication::applicationDirPath();
#else // !Q_OS_ANDROID
    QString testdata_dir = QFileInfo(QFINDTESTDATA("testProcessUniqueness")).absolutePath();
#endif
    QVERIFY2(QDir::setCurrent(testdata_dir), qPrintable("Could not chdir to " + testdata_dir));
#endif

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
    QCOMPARE(uuidA.toString(QUuid::WithoutBraces),
             QString("fc69b59e-cc34-4436-a43c-ee95d128b8c5"));
    QCOMPARE(uuidA.toString(QUuid::Id128),
             QString("fc69b59ecc344436a43cee95d128b8c5"));

    QCOMPARE(uuidB.toString(), QString("{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}"));
    QCOMPARE(uuidB.toString(QUuid::WithoutBraces),
             QString("1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b"));
    QCOMPARE(uuidB.toString(QUuid::Id128),
             QString("1ab6e93ab1cb4a87ba47ec7e99039a7b"));
}

void tst_QUuid::fromString_data()
{
    QTest::addColumn<QUuid>("expected");
    QTest::addColumn<QString>("input");

    QUuid invalid = {};

#define ROW(which, string) \
    QTest::addRow("%-38s -> %s", string, #which) << which << string
    ROW(uuidA,   "{fc69b59e-cc34-4436-a43c-ee95d128b8c5}");
    ROW(uuidA,    "fc69b59e-cc34-4436-a43c-ee95d128b8c5}");
    ROW(uuidA,   "{fc69b59e-cc34-4436-a43c-ee95d128b8c5" );
    ROW(uuidA,    "fc69b59e-cc34-4436-a43c-ee95d128b8c5" );

    ROW(uuidA,   "{fc69b59e-cc34-4436-a43c-ee95d128b8c56"); // too long (not an error!)
    ROW(invalid, "{fc69b59e-cc34-4436-a43c-ee95d128b8c"  ); // premature end (within length limits)
    ROW(invalid, " fc69b59e-cc34-4436-a43c-ee95d128b8c5}"); // leading space
    ROW(uuidA,   "{fc69b59e-cc34-4436-a43c-ee95d128b8c5 "); // trailing space (not an error!)
    ROW(invalid, "{gc69b59e-cc34-4436-a43c-ee95d128b8c5}"); // non-hex digit in 1st group
    ROW(invalid, "{fc69b59e-cp34-4436-a43c-ee95d128b8c5}"); // non-hex digit in 2nd group
    ROW(invalid, "{fc69b59e-cc34-44r6-a43c-ee95d128b8c5}"); // non-hex digit in 3rd group
    ROW(invalid, "{fc69b59e-cc34-4436-a4yc-ee95d128b8c5}"); // non-hex digit in 4th group
    ROW(invalid, "{fc69b59e-cc34-4436-a43c-ee95d128j8c5}"); // non-hex digit in last group
    ROW(invalid, "(fc69b59e-cc34-4436-a43c-ee95d128b8c5}"); // wrong initial character
    ROW(invalid, "{fc69b59e+cc34-4436-a43c-ee95d128b8c5}"); // wrong 1st separator
    ROW(invalid, "{fc69b59e-cc34*4436-a43c-ee95d128b8c5}"); // wrong 2nd separator
    ROW(invalid, "{fc69b59e-cc34-44366a43c-ee95d128b8c5}"); // wrong 3rd separator
    ROW(invalid, "{fc69b59e-cc34-4436-a43c\303\244ee95d128b8c5}"); // wrong 4th separator (&auml;)
    ROW(uuidA,   "{fc69b59e-cc34-4436-a43c-ee95d128b8c5)"); // wrong final character (not an error!)

    ROW(uuidB,   "{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}");
#undef ROW
}

void tst_QUuid::fromString()
{
    QFETCH(const QUuid, expected);
    QFETCH(const QString, input);

    const auto inputL1 = input.toLatin1();
    const auto inputU8 = input.toUtf8();

    QCOMPARE(expected, QUuid(input));
    QCOMPARE(expected, QUuid(inputU8));
    QCOMPARE(expected, QUuid(inputL1));

    QCOMPARE(expected, QUuid::fromString(input));

    // for QLatin1String, construct one whose data() is not NUL-terminated:
    const auto longerInputL1 = inputL1 + '5'; // the '5' makes the premature end check incorrectly succeed
    const auto inputL1S = QLatin1String(longerInputL1.data(), inputL1.size());
    QCOMPARE(expected, QUuid::fromString(inputL1S));
}

void tst_QUuid::toByteArray()
{
    QCOMPARE(uuidA.toByteArray(), QByteArray("{fc69b59e-cc34-4436-a43c-ee95d128b8c5}"));
    QCOMPARE(uuidA.toByteArray(QUuid::WithoutBraces),
             QByteArray("fc69b59e-cc34-4436-a43c-ee95d128b8c5"));
    QCOMPARE(uuidA.toByteArray(QUuid::Id128),
             QByteArray("fc69b59ecc344436a43cee95d128b8c5"));

    QCOMPARE(uuidB.toByteArray(), QByteArray("{1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b}"));
    QCOMPARE(uuidB.toByteArray(QUuid::WithoutBraces),
             QByteArray("1ab6e93a-b1cb-4a87-ba47-ec7e99039a7b"));
    QCOMPARE(uuidB.toByteArray(QUuid::Id128),
             QByteArray("1ab6e93ab1cb4a87ba47ec7e99039a7b"));
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
    QCOMPARE(uuidA, copy);

    QUuid assigned;
    assigned = uuidA;
    QCOMPARE(uuidA, assigned);
}


void tst_QUuid::notEqual()
{
    QVERIFY( uuidA != uuidB );
}

void tst_QUuid::cpp11() {
#ifdef Q_COMPILER_UNIFORM_INIT
    // "{fc69b59e-cc34-4436-a43c-ee95d128b8c5}" cf, initTestCase
    Q_DECL_CONSTEXPR QUuid u1{0xfc69b59e, 0xcc34, 0x4436, 0xa4, 0x3c, 0xee, 0x95, 0xd1, 0x28, 0xb8, 0xc5};
    Q_DECL_CONSTEXPR QUuid u2 = {0xfc69b59e, 0xcc34, 0x4436, 0xa4, 0x3c, 0xee, 0x95, 0xd1, 0x28, 0xb8, 0xc5};
    Q_UNUSED(u1);
    Q_UNUSED(u2);
#else
    QSKIP("This compiler is not in C++11 mode or it doesn't support uniform initialization");
#endif
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
    QVERIFY(  uuidB <  uuidA);
    QVERIFY(  uuidB <= uuidA);
    QVERIFY(!(uuidA <  uuidB) );
    QVERIFY(!(uuidA <= uuidB));

    QUuid null_uuid;
    QVERIFY(null_uuid < uuidA); // Null uuid is always less than a valid one
    QVERIFY(null_uuid <= uuidA);

    QVERIFY(null_uuid <= null_uuid);
    QVERIFY(uuidA <= uuidA);
}


void tst_QUuid::more()
{
    QVERIFY(  uuidA >  uuidB);
    QVERIFY(  uuidA >= uuidB);
    QVERIFY(!(uuidB >  uuidA));
    QVERIFY(!(uuidB >= uuidA));

    QUuid null_uuid;
    QVERIFY(!(null_uuid >  uuidA)); // Null uuid is always less than a valid one
    QVERIFY(!(null_uuid >= uuidA));

    QVERIFY(null_uuid >= null_uuid);
    QVERIFY(uuidA >= uuidA);
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

void tst_QUuid::processUniqueness()
{
#if !QT_CONFIG(process)
    QSKIP("No qprocess support", SkipAll);
#else
#ifdef Q_OS_ANDROID
    QSKIP("This test crashes on Android");
#endif
    QProcess process;
    QString processOneOutput;
    QString processTwoOutput;

    // Start it once
#ifdef Q_OS_MAC
    process.start("testProcessUniqueness/testProcessUniqueness.app");
#elif defined(Q_OS_ANDROID)
    process.start("libtestProcessUniqueness.so");
#else
    process.start("testProcessUniqueness/testProcessUniqueness");
#endif
    QVERIFY(process.waitForFinished());
    processOneOutput = process.readAllStandardOutput();

    // Start it twice
#ifdef Q_OS_MAC
    process.start("testProcessUniqueness/testProcessUniqueness.app");
#elif defined(Q_OS_ANDROID)
    process.start("libtestProcessUniqueness.so");
#else
    process.start("testProcessUniqueness/testProcessUniqueness");
#endif
    QVERIFY(process.waitForFinished());
    processTwoOutput = process.readAllStandardOutput();

    // They should be *different*!
    QVERIFY(processOneOutput != processTwoOutput);
#endif
}

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

    // QUuid -> QString
    QVERIFY(v.canConvert<QString>());
    QCOMPARE(v.toString(), uuid.toString());
    QCOMPARE(v.value<QString>(), uuid.toString());

    // QUuid -> QByteArray
    QVERIFY(v.canConvert<QByteArray>());
    QCOMPARE(v.toByteArray(), uuid.toByteArray());
    QCOMPARE(v.value<QByteArray>(), uuid.toByteArray());

    QVERIFY(!v.canConvert<int>());
    QVERIFY(!v.canConvert<QStringList>());

    // try reverse conversion QString -> QUuid
    QVariant sv = QVariant::fromValue(uuid.toString());
    QCOMPARE(sv.type(), QVariant::String);
    QVERIFY(sv.canConvert<QUuid>());
    QCOMPARE(sv.value<QUuid>(), uuid);

    // QString -> QUuid
    {
        QVariant sv = QVariant::fromValue(uuid.toByteArray());
        QCOMPARE(sv.type(), QVariant::ByteArray);
        QVERIFY(sv.canConvert<QUuid>());
        QCOMPARE(sv.value<QUuid>(), uuid);
    }
}

void tst_QUuid::darwinTypes()
{
#ifndef Q_OS_DARWIN
    QSKIP("This is a Darwin-only test");
#else
    extern void tst_QUuid_darwinTypes(); // in tst_quuid_darwin.mm
    tst_QUuid_darwinTypes();
#endif
}

QTEST_MAIN(tst_QUuid)
#include "tst_quuid.moc"
