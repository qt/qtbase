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
#include <QtGui/QBitmap>
#include <QtGui/QPalette>
#include <QtGui/QPixmap>
#include <QtGui/QPicture>
#include <QtGui/QTextLength>
#include <QtGui/QPainter>
#include <QtGui/QPen>

#include "../../../qtest-config.h"

class tst_QDataStream : public QObject
{
Q_OBJECT

public:
    void stream_data(int noOfElements);

public slots:
    void cleanupTestCase();

private slots:
    void getSetCheck();
    void stream_bool_data();
    void stream_bool();

    void stream_QBitArray_data();
    void stream_QBitArray();

    void stream_QBrush_data();
    void stream_QBrush();

    void stream_QColor_data();
    void stream_QColor();

    void stream_QByteArray_data();
    void stream_QByteArray();

#ifndef QTEST_NO_CURSOR
    void stream_QCursor_data();
    void stream_QCursor();
#endif

    void stream_QDate_data();
    void stream_QDate();

    void stream_QTime_data();
    void stream_QTime();

    void stream_QDateTime_data();
    void stream_QDateTime();

    void stream_QFont_data();
    void stream_QFont();

    void stream_QImage_data();
    void stream_QImage();

    void stream_QPen_data();
    void stream_QPen();

#ifndef Q_OS_WINCE
    void stream_QPixmap_data();
    void stream_QPixmap();
#endif

    void stream_QPoint_data();
    void stream_QPoint();

    void stream_QRect_data();
    void stream_QRect();

    void stream_QPolygon_data();
    void stream_QPolygon();

    void stream_QRegion_data();
    void stream_QRegion();

    void stream_QSize_data();
    void stream_QSize();

    void stream_QString_data();
    void stream_QString();

    void stream_QRegExp_data();
    void stream_QRegExp();

    void stream_Map_data();
    void stream_Map();

    void stream_Hash_data();
    void stream_Hash();

    void stream_qint64_data();
    void stream_qint64();

#ifndef Q_OS_WINCE
    void stream_QIcon_data();
    void stream_QIcon();
#endif

    void stream_QEasingCurve_data();
    void stream_QEasingCurve();

    void stream_atEnd_data();
    void stream_atEnd();

    void stream_writeError();

    void stream_QByteArray2();

    void setVersion_data();
    void setVersion();

    void skipRawData_data();
    void skipRawData();

    void status_qint8_data();
    void status_qint8();
    void status_qint16_data();
    void status_qint16();
    void status_qint32_data();
    void status_qint32();
    void status_qint64_data();
    void status_qint64();

    void status_float_data();
    void status_float();
    void status_double_data();
    void status_double();

    void status_charptr_QByteArray_data();
    void status_charptr_QByteArray();

    void status_QString_data();
    void status_QString();

    void status_QBitArray_data();
    void status_QBitArray();

    void status_QHash_QMap();

    void status_QLinkedList_QList_QVector();

    void streamToAndFromQByteArray();

    void streamRealDataTypes();

    void floatingPointPrecision();

    void compatibility_Qt3();
    void compatibility_Qt2();

    void floatingPointNaN();

private:
    void writebool(QDataStream *s);
    void writeQBitArray(QDataStream *s);
    void writeQBrush(QDataStream *s);
    void writeQColor(QDataStream *s);
    void writeQByteArray(QDataStream *s);
#ifndef QTEST_NO_CURSOR
    void writeQCursor(QDataStream *s);
#endif
    void writeQWaitCursor(QDataStream *s);
    void writeQDate(QDataStream *s);
    void writeQTime(QDataStream *s);
    void writeQDateTime(QDataStream *s);
    void writeQFont(QDataStream *s);
    void writeQImage(QDataStream *s);
    void writeQPen(QDataStream *s);
    void writeQPixmap(QDataStream *s);
    void writeQPoint(QDataStream *s);
    void writeQRect(QDataStream *s);
    void writeQPolygon(QDataStream *s);
    void writeQRegion(QDataStream *s);
    void writeQSize(QDataStream *s);
    void writeQString(QDataStream* dev);
    void writeQRegExp(QDataStream* dev);
    void writeMap(QDataStream* dev);
    void writeHash(QDataStream* dev);
    void writeqint64(QDataStream *s);
    void writeQIcon(QDataStream *s);
    void writeQEasingCurve(QDataStream *s);

    void readbool(QDataStream *s);
    void readQBitArray(QDataStream *s);
    void readQBrush(QDataStream *s);
    void readQColor(QDataStream *s);
    void readQByteArray(QDataStream *s);
#ifndef QTEST_NO_CURSOR
    void readQCursor(QDataStream *s);
#endif
    void readQDate(QDataStream *s);
    void readQTime(QDataStream *s);
    void readQDateTime(QDataStream *s);
    void readQFont(QDataStream *s);
    void readQImage(QDataStream *s);
    void readQPen(QDataStream *s);
    void readQPixmap(QDataStream *s);
    void readQPoint(QDataStream *s);
    void readQRect(QDataStream *s);
    void readQPolygon(QDataStream *s);
    void readQRegion(QDataStream *s);
    void readQSize(QDataStream *s);
    void readQString(QDataStream *s);
    void readQRegExp(QDataStream *s);
    void readMap(QDataStream *s);
    void readHash(QDataStream *s);
    void readqint64(QDataStream *s);
    void readQIcon(QDataStream *s);
    void readQEasingCurve(QDataStream *s);
};

static int NColorRoles[] = {
    QPalette::NoRole,              // No Version
    QPalette::NoRole,              // Qt_1_0
    QPalette::HighlightedText + 1, // Qt_2_0
    QPalette::HighlightedText + 1, // Qt_2_1
    QPalette::LinkVisited + 1,     // Qt_3_0
    QPalette::HighlightedText + 1, // Qt_3_1
    QPalette::HighlightedText + 1, // Qt_3_3
    QPalette::HighlightedText + 1, // Qt_4_0, Qt_4_1
    QPalette::HighlightedText + 1, // Qt_4_2
    QPalette::AlternateBase + 1,   // Qt_4_3
    QPalette::ToolTipText + 1,     // Qt_4_4
    QPalette::ToolTipText + 1,     // Qt_4_5
    QPalette::ToolTipText + 1,     // Qt_4_6
    QPalette::ToolTipText + 1,     // Qt_5_0
    QPalette::ToolTipText + 1,     // Qt_5_1
    QPalette::ToolTipText + 1,     // Qt_5_2
    0                              // add the correct value for Qt_5_3 here later
};

// Testing get/set functions
void tst_QDataStream::getSetCheck()
{
    QDataStream obj1;
    // QIODevice * QDataStream::device()
    // void QDataStream::setDevice(QIODevice *)
    QFile *var1 = new QFile;
    obj1.setDevice(var1);
    QCOMPARE((QIODevice *)var1, (QIODevice *)obj1.device());
    obj1.setDevice((QIODevice *)0);
    QCOMPARE((QIODevice *)0, (QIODevice *)obj1.device());
    delete var1;

    // Status QDataStream::status()
    // void QDataStream::setStatus(Status)
    obj1.setStatus(QDataStream::Ok);
    QCOMPARE(QDataStream::Ok, obj1.status());
    obj1.setStatus(QDataStream::ReadPastEnd);
    QCOMPARE(QDataStream::ReadPastEnd, obj1.status());
    obj1.resetStatus();
    obj1.setStatus(QDataStream::ReadCorruptData);
    QCOMPARE(QDataStream::ReadCorruptData, obj1.status());
}

void tst_QDataStream::cleanupTestCase()
{
    QFile::remove(QLatin1String("qdatastream.out"));
    QFile::remove(QLatin1String("datastream.tmp"));
}

static int dataIndex(const QString &tag)
{
    int pos = tag.lastIndexOf("_");
    if (pos >= 0) {
        int ret = 0;
        QString count = tag.mid(pos + 1);
        bool ok;
        ret = count.toInt(&ok);
        if (ok)
            return ret;
    }
    return -1;
}

static const char * const devices[] = {
    "file",
    "bytearray",
    "buffer",
    0
};

/*
    IMPORTANT.
    In this testcase i follow a different approach than usual: I don't use the full power of
    QtTestTable and QtTestData. This is done deliberately because QtTestData uses a QDataStream
    itself to handle its data. So it would be a bit inapropriate to fully rely on QtTestData in this
    testcase.
    I do use QString in QtTestData because this is thouroughly tested in the selftest.
*/
void tst_QDataStream::stream_data(int noOfElements)
{
    QTest::addColumn<QString>("device");
    QTest::addColumn<QString>("byteOrder");

    for (int d=0; devices[d] != 0; d++) {
        QString device = devices[d];
        for (int b=0; b<2; b++) {
            QString byte_order = b == 0 ? "BigEndian" : "LittleEndian";

            QString tag = device + "_" + byte_order;
            for (int e=0; e<noOfElements; e++) {
                QTest::newRow(qPrintable(tag + QString("_%1").arg(e))) << device << QString(byte_order);
            }
        }
    }
}

static const char* open_xpm[]={
"16 13 6 1",
". c None",
"b c #ffff00",
"d c #000000",
"* c #999999",
"c c #cccccc",
"a c #ffffff",
"...*****........",
"..*aaaaa*.......",
".*abcbcba******.",
".*acbcbcaaaaaa*d",
".*abcbcbcbcbcb*d",
"*************b*d",
"*aaaaaaaaaa**c*d",
"*abcbcbcbcbbd**d",
".*abcbcbcbcbcd*d",
".*acbcbcbcbcbd*d",
"..*acbcbcbcbb*dd",
"..*************d",
"...ddddddddddddd"};

#define STREAM_IMPL(TYPE) \
    QFETCH(QString, device); \
    if (device == "bytearray") { \
        QByteArray ba; \
        QDataStream sout(&ba, QIODevice::WriteOnly); \
        write##TYPE(&sout); \
        QDataStream sin(&ba, QIODevice::ReadOnly); \
        read##TYPE(&sin); \
    } else if (device == "file") { \
        QString fileName = "qdatastream.out"; \
        QFile fOut(fileName); \
        QVERIFY(fOut.open(QIODevice::WriteOnly)); \
        QDataStream sout(&fOut); \
        write##TYPE(&sout); \
        fOut.close(); \
        QFile fIn(fileName); \
        QVERIFY(fIn.open(QIODevice::ReadOnly)); \
        QDataStream sin(&fIn); \
        read##TYPE(&sin); \
        fIn.close(); \
    } else if (device == "buffer") { \
        QByteArray ba(10000, '\0'); \
        QBuffer bOut(&ba); \
        bOut.open(QIODevice::WriteOnly); \
        QDataStream sout(&bOut); \
        write##TYPE(&sout); \
        bOut.close(); \
        QBuffer bIn(&ba); \
        bIn.open(QIODevice::ReadOnly); \
        QDataStream sin(&bIn); \
        read##TYPE(&sin); \
        bIn.close(); \
    }

// ************************************

static QString stringData(int index)
{
    switch (index) {
    case 0: return QString();
    case 1: return QString("");
    case 2: return QString("A");
    case 3: return QString("ABCDE FGHI");
    case 4: return QString("This is a long string");
    case 5: return QString("And again a string with a \nCRLF");
    case 6: return QString("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRESTUVWXYZ 1234567890 ~`!@#$%^&*()_-+={[}]|\\:;\"'<,>.?/");
    }
    return QString("foo");
}
#define MAX_QSTRING_DATA 7

void tst_QDataStream::stream_QString_data()
{
    stream_data(MAX_QSTRING_DATA);
}

void tst_QDataStream::stream_QString()
{
    STREAM_IMPL(QString);
}

void tst_QDataStream::writeQString(QDataStream* s)
{
    QString test(stringData(dataIndex(QTest::currentDataTag())));
    *s << test;
    *s << QString("Her er det noe tekst");
    *s << test;
    *s << QString();
    *s << test;
    *s << QString("");
    *s << test;
    *s << QString("nonempty");
    *s << test;
}

void tst_QDataStream::readQString(QDataStream *s)
{
    QString S;
    QString test(stringData(dataIndex(QTest::currentDataTag())));

    *s >> S;
    QCOMPARE(S, test);
    *s >> S;
    QCOMPARE(S, QString("Her er det noe tekst"));
    *s >> S;
    QCOMPARE(S, test);
    *s >> S;
    QVERIFY(S.isNull());
    *s >> S;
    QCOMPARE(S, test);
    *s >> S;
    QVERIFY(S.isEmpty());
    *s >> S;
    QCOMPARE(S, test);
    *s >> S;
    QCOMPARE(S, QString("nonempty"));
    *s >> S;
    QCOMPARE(S, test);
}

// ************************************

static QRegExp QRegExpData(int index)
{
    switch (index) {
    case 0: return QRegExp();
    case 1: return QRegExp("");
    case 2: return QRegExp("A", Qt::CaseInsensitive);
    case 3: return QRegExp("ABCDE FGHI", Qt::CaseSensitive, QRegExp::Wildcard);
    case 4: return QRegExp("This is a long string", Qt::CaseInsensitive, QRegExp::FixedString);
    case 5: return QRegExp("And again a string with a \nCRLF", Qt::CaseInsensitive, QRegExp::RegExp);
    case 6:
        {
            QRegExp rx("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRESTUVWXYZ 1234567890 ~`!@#$%^&*()_-+={[}]|\\:;\"'<,>.?/");
            rx.setMinimal(true);
            return rx;
        }
    }
    return QRegExp("foo");
}
#define MAX_QREGEXP_DATA 7

void tst_QDataStream::stream_QRegExp_data()
{
    stream_data(MAX_QREGEXP_DATA);
}

void tst_QDataStream::stream_QRegExp()
{
    STREAM_IMPL(QRegExp);
}

void tst_QDataStream::writeQRegExp(QDataStream* s)
{
    QRegExp test(QRegExpData(dataIndex(QTest::currentDataTag())));
    *s << test;
    *s << QString("Her er det noe tekst");
    *s << test;
    *s << QString("nonempty");
    *s << test;
    *s << QVariant(test);
}

void tst_QDataStream::readQRegExp(QDataStream *s)
{
    QRegExp R;
    QString S;
    QVariant V;
    QRegExp test(QRegExpData(dataIndex(QTest::currentDataTag())));

    *s >> R;
    QCOMPARE(R, test);
    *s >> S;
    QCOMPARE(S, QString("Her er det noe tekst"));
    *s >> R;
    QCOMPARE(R, test);
    *s >> S;
    QCOMPARE(S, QString("nonempty"));
    *s >> R;
    QCOMPARE(R, test);
    *s >> V;
    QVERIFY(V.type() == QVariant::RegExp);
    QCOMPARE(V.toRegExp(), test);
}

// ************************************

typedef QMap<int, QString> Map;

static Map MapData(int index)
{
    Map map;

    switch (index) {
    case 0:
    default:
        break;
    case 1:
        map.insert(1, "a");
        map.insert(2, "bbb");
        map.insert(3, "cccccc");
        break;
    case 2:
        map.insert(1, "a");
        map.insert(2, "one");
        map.insertMulti(2, "two");
        map.insertMulti(2, "three");
        map.insert(3, "cccccc");
    }
    return map;
}
#define MAX_MAP_DATA 3

void tst_QDataStream::stream_Map_data()
{
    stream_data(MAX_MAP_DATA);
}

void tst_QDataStream::stream_Map()
{
    STREAM_IMPL(Map);
}

void tst_QDataStream::writeMap(QDataStream* s)
{
    Map test(MapData(dataIndex(QTest::currentDataTag())));
    *s << test;
    *s << test;
}

void tst_QDataStream::readMap(QDataStream *s)
{
    Map S;
    Map test(MapData(dataIndex(QTest::currentDataTag())));

    *s >> S;
    QCOMPARE(S, test);
    *s >> S;
    QCOMPARE(S, test);
}

// ************************************

typedef QHash<int, QString> Hash;

static Hash HashData(int index)
{
    Hash map;

    switch (index) {
    case 0:
    default:
        break;
    case 1:
        map.insert(1, "a");
        map.insert(2, "bbb");
        map.insert(3, "cccccc");
        break;
    case 2:
        map.insert(1, "a");
        map.insert(2, "one");
        map.insertMulti(2, "two");
        map.insertMulti(2, "three");
        map.insert(3, "cccccc");
    }
    return map;
}
#define MAX_HASH_DATA 3

void tst_QDataStream::stream_Hash_data()
{
    stream_data(MAX_HASH_DATA);
}

void tst_QDataStream::stream_Hash()
{
    STREAM_IMPL(Hash);
}

void tst_QDataStream::writeHash(QDataStream* s)
{
    Hash test(HashData(dataIndex(QTest::currentDataTag())));
    *s << test;
    *s << test;
}

void tst_QDataStream::readHash(QDataStream *s)
{
    Hash S;
    Hash test(HashData(dataIndex(QTest::currentDataTag())));

    *s >> S;
    QCOMPARE(S, test);
    *s >> S;
    QCOMPARE(S, test);
}

// ************************************

static QEasingCurve QEasingCurveData(int index)
{
    QEasingCurve easing;

    switch (index) {
    case 0:
    default:
        break;
    case 1:
        easing.setType(QEasingCurve::Linear);
        break;
    case 2:
        easing.setType(QEasingCurve::OutCubic);
        break;
    case 3:
        easing.setType(QEasingCurve::InOutSine);
        break;
    case 4:
        easing.setType(QEasingCurve::InOutElastic);
        easing.setPeriod(1.5);
        easing.setAmplitude(2.0);
        break;
    case 5:
        easing.setType(QEasingCurve::OutInBack);
        break;
    case 6:
        easing.setType(QEasingCurve::OutCurve);
        break;
    case 7:
        easing.setType(QEasingCurve::InOutBack);
        easing.setOvershoot(0.5);
        break;
    }
    return easing;
}
#define MAX_EASING_DATA 8

void tst_QDataStream::stream_QEasingCurve_data()
{
    stream_data(MAX_EASING_DATA);
}

void tst_QDataStream::stream_QEasingCurve()
{
    STREAM_IMPL(QEasingCurve);
}

void tst_QDataStream::writeQEasingCurve(QDataStream* s)
{
    QEasingCurve test(QEasingCurveData(dataIndex(QTest::currentDataTag())));
    *s << test;
}

void tst_QDataStream::readQEasingCurve(QDataStream *s)
{
    QEasingCurve S;
    QEasingCurve expected(QEasingCurveData(dataIndex(QTest::currentDataTag())));

    *s >> S;
    QCOMPARE(S, expected);
}

// ************************************

// contains some quint64 testing as well

#define MAX_qint64_DATA 4

static qint64 qint64Data(int index)
{
    switch (index) {
    case 0: return qint64(0);
    case 1: return qint64(1);
    case 2: return qint64(-1);
    case 3: return qint64(1) << 40;
    case MAX_qint64_DATA: return -(qint64(1) << 40);
    }

    return -1;
}

void tst_QDataStream::stream_qint64_data()
{
    stream_data(MAX_qint64_DATA+1);
}

void tst_QDataStream::stream_qint64()
{
    STREAM_IMPL(qint64);
}

void tst_QDataStream::writeqint64(QDataStream* s)
{
    qint64 test = qint64Data(dataIndex(QTest::currentDataTag()));
    *s << test;
    *s << int(1);
    *s << (quint64)test;
}

void tst_QDataStream::readqint64(QDataStream *s)
{
    qint64 test = qint64Data(dataIndex(QTest::currentDataTag()));
    qint64 i64;
    quint64 ui64;
    int i;
    *s >> i64;
    QCOMPARE(i64, test);
    *s >> i;
    QCOMPARE(i, int(1));
    *s >> ui64;
    QCOMPARE(ui64, (quint64)test);
}

// ************************************

static bool boolData(int index)
{
    switch (index) {
    case 0: return true;
    case 1: return false;
    case 2: return bool(2);
    case 3: return bool(-1);
    case 4: return bool(127);
    }

    return false;
}

void tst_QDataStream::stream_bool_data()
{
    stream_data(5);
}

void tst_QDataStream::stream_bool()
{
    STREAM_IMPL(bool);
}

void tst_QDataStream::writebool(QDataStream *s)
{
    bool d1 = boolData(dataIndex(QTest::currentDataTag()));
    *s << d1;
}

void tst_QDataStream::readbool(QDataStream *s)
{
    bool expected = boolData(dataIndex(QTest::currentDataTag()));

    bool d1;
    *s >> d1;
    QVERIFY(d1 == expected);
}

// ************************************

static void QBitArrayData(QBitArray *b, int index)
{
    QString filler = "";
    switch (index) {
    case 0: filler = ""; break;
    case 1: filler = ""; break;
    case 2: filler = "0"; break;
    case 3: filler = "1"; break;
    case 4: filler = "0000"; break;
    case 5: filler = "0001"; break;
    case 6: filler = "0010"; break;
    case 7: filler = "0100"; break;
    case 8: filler = "1000"; break;
    case 9: filler = "1111"; break;
    case 10: filler = "00000000"; break;
    case 11: filler = "00000001"; break;
    case 12: filler = "11111111"; break;
    case 13: filler = "000000001"; break;
    case 14: filler = "000000000001"; break;
    case 15: filler = "0000000000000001"; break;
    case 16: filler = "0101010101010101010101010101010101010101010101010101010101010101"; break;
    case 17: filler = "1010101010101010101010101010101010101010101010101010101010101010"; break;
    case 18: filler = "1111111111111111111111111111111111111111111111111111111111111111"; break;
    }

    b->resize(filler.length());
    b->fill(0); // reset all bits to zero

    for (int i = 0; i < filler.length(); ++i) {
        if (filler.at(i) == '1')
            b->setBit(i, true);
    }
}

void tst_QDataStream::stream_QBitArray_data()
{
    stream_data(19);
}

void tst_QDataStream::stream_QBitArray()
{
    STREAM_IMPL(QBitArray);
}

void tst_QDataStream::writeQBitArray(QDataStream *s)
{
    QBitArray d1;
    QBitArrayData(&d1, dataIndex(QTest::currentDataTag()));
    *s << d1;
}

void tst_QDataStream::readQBitArray(QDataStream *s)
{
    QBitArray expected;
    QBitArrayData(&expected, dataIndex(QTest::currentDataTag()));

    QBitArray d1;
    *s >> d1;
    QVERIFY(d1 == expected);
}

// ************************************

static QBrush qBrushData(int index)
{
    switch (index) {
    case 0: return QBrush(Qt::NoBrush);
    case 1: return QBrush(Qt::SolidPattern);
    case 2: return QBrush(Qt::Dense7Pattern);
    case 3: return QBrush(Qt::red, Qt::NoBrush);
    case 4: return QBrush(Qt::green, Qt::SolidPattern);
    case 5: return QBrush(Qt::blue, Qt::Dense7Pattern);
    case 6:
        {
            QPixmap pm(open_xpm);
            QBrush custom(Qt::black, pm);
            return custom;
        }
    case 7:
        QLinearGradient gradient(QPointF(2.718, 3.142), QPointF(3.1337, 42));
        gradient.setCoordinateMode(QGradient::ObjectBoundingMode);
        gradient.setSpread(QGradient::ReflectSpread);
        gradient.setInterpolationMode(QGradient::ComponentInterpolation);
        gradient.setColorAt(0.2, Qt::red);
        gradient.setColorAt(0.6, Qt::transparent);
        gradient.setColorAt(0.8, Qt::blue);
        return QBrush(gradient);
    }

    return QBrush(Qt::NoBrush);
}

void tst_QDataStream::stream_QBrush_data()
{
    stream_data(8);
}

void tst_QDataStream::stream_QBrush()
{
    if (QString(QTest::currentDataTag()).endsWith("6"))
        QSKIP("Custom brushes don't seem to be supported with QDataStream");

    STREAM_IMPL(QBrush);
}

void tst_QDataStream::writeQBrush(QDataStream *s)
{
    QBrush brush = qBrushData(dataIndex(QTest::currentDataTag()));
    *s << brush;
}

void tst_QDataStream::readQBrush(QDataStream *s)
{
    QBrush d2;
    *s >> d2;

    QBrush brush = qBrushData(dataIndex(QTest::currentDataTag()));
    QVERIFY(d2 == brush);
}

// ************************************

static QColor QColorData(int index)
{
    switch (index) {
    case 0: return QColor(0,0,0);
    case 1: return QColor(0,0,0);
    case 2: return QColor(0,0,0);
    case 3: return QColor(0,0,0);
    case 4: return QColor(0,0,0);
    case 5: return QColor(0,0,0);
    case 6: return QColor(0,0,0);
    case 7: return QColor(0,0,0);
    }

    return QColor(0,0,0);
}

void tst_QDataStream::stream_QColor_data()
{
    stream_data(8);
}

void tst_QDataStream::stream_QColor()
{
    STREAM_IMPL(QColor);
}

void tst_QDataStream::writeQColor(QDataStream *s)
{
    QColor d3(QColorData(dataIndex(QTest::currentDataTag())));
    *s << d3;
}

void tst_QDataStream::readQColor(QDataStream *s)
{
    QColor test(QColorData(dataIndex(QTest::currentDataTag())));
    QColor d3;
    *s >> d3;
    QVERIFY(d3 == test);
}


// ************************************

static QByteArray qByteArrayData(int index)
{
    switch (index) {
    case 0: return QByteArray();
    case 1: return QByteArray("");
    case 2: return QByteArray("foo");
    case 3: return QByteArray("foo bar");
    case 4: return QByteArray("two\nlines");
    case 5: return QByteArray("ABCDEFG");
    case 6: return QByteArray("baec zxv 123"); // kept for nostalgic reasons
    case 7: return QByteArray("jbc;UBC;jd clhdbcahd vcbd vgdv dhvb laifv kadf jkhfbvljd khd lhvjh ");
    }

    return QByteArray("foo");
}

void tst_QDataStream::stream_QByteArray_data()
{
    stream_data(8);
}

void tst_QDataStream::stream_QByteArray()
{
    STREAM_IMPL(QByteArray);
}

void tst_QDataStream::writeQByteArray(QDataStream *s)
{
    QByteArray d4(qByteArrayData(dataIndex(QTest::currentDataTag())));
    *s << d4;
}

void tst_QDataStream::readQByteArray(QDataStream *s)
{
    QByteArray test(qByteArrayData(dataIndex(QTest::currentDataTag())));
    QByteArray d4;
    *s >> d4;
    QCOMPARE(d4, test);
}

// ************************************
#ifndef QTEST_NO_CURSOR
static QCursor qCursorData(int index)
{
    switch (index) {
    case 0: return QCursor(Qt::ArrowCursor);
    case 1: return QCursor(Qt::WaitCursor);
    case 2: return QCursor(Qt::BitmapCursor);
    case 3: return QCursor(Qt::BlankCursor);
    case 4: return QCursor(Qt::BlankCursor);
    case 5: return QCursor(QPixmap(open_xpm), 1, 1);
    case 6: { QPixmap pm(open_xpm); return QCursor(QBitmap(pm), pm.mask(), 3, 4); }
    case 7: return QCursor(QPixmap(open_xpm), -1, 5);
    case 8: return QCursor(QPixmap(open_xpm), 5, -1);
    }

    return QCursor();
}
#endif

#ifndef QTEST_NO_CURSOR
void tst_QDataStream::stream_QCursor_data()
{
    stream_data(9);
}
#endif

#ifndef QTEST_NO_CURSOR
void tst_QDataStream::stream_QCursor()
{
    STREAM_IMPL(QCursor);
}
#endif

#ifndef QTEST_NO_CURSOR
void tst_QDataStream::writeQCursor(QDataStream *s)
{
    QCursor d5(qCursorData(dataIndex(QTest::currentDataTag())));
    *s << d5;
}
#endif

#ifndef QTEST_NO_CURSOR
void tst_QDataStream::readQCursor(QDataStream *s)
{
    QCursor test(qCursorData(dataIndex(QTest::currentDataTag())));
    QCursor d5;
    *s >> d5;

    QVERIFY(d5.shape() == test.shape()); //## lacks operator==
    QVERIFY(d5.hotSpot() == test.hotSpot());
    QVERIFY((d5.bitmap() != 0 && test.bitmap() != 0) || (d5.bitmap() == 0 && test.bitmap() == 0));
    if (d5.bitmap() != 0) {
        QPixmap actual = *(d5.bitmap());
        QPixmap expected = *(test.bitmap());
        QCOMPARE(actual, expected);
    }
    QVERIFY((d5.mask() != 0 && test.mask() != 0) || (d5.mask() == 0 && test.mask() == 0));
    if (d5.mask() != 0) {
        QPixmap actual = *(d5.mask());
        QPixmap expected = *(test.mask());
        QCOMPARE(actual, expected);
    }
}
#endif

// ************************************

static QDate qDateData(int index)
{
    switch (index) {
    case 0: return QDate(1752, 9, 14); // the first valid date
    case 1: return QDate(1900, 1, 1);
    case 2: return QDate(1976, 4, 5);
    case 3: return QDate(1960, 5, 27);
    case 4: return QDate(1999, 12, 31); // w2k effects?
    case 5: return QDate(2000, 1, 1);
    case 6: return QDate(2050, 1, 1);// test some values far in the future too
    case 7: return QDate(3001, 12, 31);
    case 8: return QDate(4002, 1, 1);
    case 9: return QDate(4003, 12, 31);
    case 10: return QDate(5004, 1, 1);
    case 11: return QDate(5005, 12, 31);
    case 12: return QDate(6006, 1, 1);
    case 13: return QDate(6007, 12, 31);
    case 14: return QDate(7008, 1, 1);
    case 15: return QDate(7009, 12, 31);
    }
    return QDate();
}
#define MAX_QDATE_DATA 16

void tst_QDataStream::stream_QDate_data()
{
    stream_data(MAX_QDATE_DATA);
}

void tst_QDataStream::stream_QDate()
{
    STREAM_IMPL(QDate);
}

void tst_QDataStream::writeQDate(QDataStream *s)
{
    QDate d6(qDateData(dataIndex(QTest::currentDataTag())));
    *s << d6;
}

void tst_QDataStream::readQDate(QDataStream *s)
{
    QDate test(qDateData(dataIndex(QTest::currentDataTag())));
    QDate d6;
    *s >> d6;
    QVERIFY(d6 == test);
}

// ************************************

static QTime qTimeData(int index)
{
    switch (index) {
    case 0 : return QTime(0, 0, 0, 0);
    case 1 : return QTime(0, 0, 0, 1);
    case 2 : return QTime(0, 0, 0, 99);
    case 3 : return QTime(0, 0, 0, 100);
    case 4 : return QTime(0, 0, 0, 999);
    case 5 : return QTime(0, 0, 1, 0);
    case 6 : return QTime(0, 0, 1, 1);
    case 7 : return QTime(0, 0, 1, 99);
    case 8 : return QTime(0, 0, 1, 100);
    case 9 : return QTime(0, 0, 1, 999);
    case 10: return QTime(0, 0, 59, 0);
    case 11: return QTime(0, 0, 59, 1);
    case 12: return QTime(0, 0, 59, 99);
    case 13: return QTime(0, 0, 59, 100);
    case 14: return QTime(0, 0, 59, 999);
    case 15: return QTime(0, 59, 0, 0);
    case 16: return QTime(0, 59, 0, 1);
    case 17: return QTime(0, 59, 0, 99);
    case 18: return QTime(0, 59, 0, 100);
    case 19: return QTime(0, 59, 0, 999);
    case 20: return QTime(0, 59, 1, 0);
    case 21: return QTime(0, 59, 1, 1);
    case 22: return QTime(0, 59, 1, 99);
    case 23: return QTime(0, 59, 1, 100);
    case 24: return QTime(0, 59, 1, 999);
    case 25: return QTime(0, 59, 59, 0);
    case 26: return QTime(0, 59, 59, 1);
    case 27: return QTime(0, 59, 59, 99);
    case 28: return QTime(0, 59, 59, 100);
    case 29: return QTime(0, 59, 59, 999);
    case 30: return QTime(23, 0, 0, 0);
    case 31: return QTime(23, 0, 0, 1);
    case 32: return QTime(23, 0, 0, 99);
    case 33: return QTime(23, 0, 0, 100);
    case 34: return QTime(23, 0, 0, 999);
    case 35: return QTime(23, 0, 1, 0);
    case 36: return QTime(23, 0, 1, 1);
    case 37: return QTime(23, 0, 1, 99);
    case 38: return QTime(23, 0, 1, 100);
    case 39: return QTime(23, 0, 1, 999);
    case 40: return QTime(23, 0, 59, 0);
    case 41: return QTime(23, 0, 59, 1);
    case 42: return QTime(23, 0, 59, 99);
    case 43: return QTime(23, 0, 59, 100);
    case 44: return QTime(23, 0, 59, 999);
    case 45: return QTime(23, 59, 0, 0);
    case 46: return QTime(23, 59, 0, 1);
    case 47: return QTime(23, 59, 0, 99);
    case 48: return QTime(23, 59, 0, 100);
    case 49: return QTime(23, 59, 0, 999);
    case 50: return QTime(23, 59, 1, 0);
    case 51: return QTime(23, 59, 1, 1);
    case 52: return QTime(23, 59, 1, 99);
    case 53: return QTime(23, 59, 1, 100);
    case 54: return QTime(23, 59, 1, 999);
    case 55: return QTime(23, 59, 59, 0);
    case 56: return QTime(23, 59, 59, 1);
    case 57: return QTime(23, 59, 59, 99);
    case 58: return QTime(23, 59, 59, 100);
    case 59: return QTime(23, 59, 59, 999);
    }
    return QTime(0, 0, 0);
}
#define MAX_QTIME_DATA 60

void tst_QDataStream::stream_QTime_data()
{
    stream_data(MAX_QTIME_DATA);
}

void tst_QDataStream::stream_QTime()
{
    STREAM_IMPL(QTime);
}

void tst_QDataStream::writeQTime(QDataStream *s)
{
    QTime d7 = qTimeData(dataIndex(QTest::currentDataTag()));
    *s << d7;
}

void tst_QDataStream::readQTime(QDataStream *s)
{
    QTime test = qTimeData(dataIndex(QTest::currentDataTag()));
    QTime d7;
    *s >> d7;
    QVERIFY(d7 == test);
}

// ************************************

static QDateTime qDateTimeData(int index)
{
    switch (index) {
    case 0: return QDateTime(QDate(1900, 1, 1), QTime(0,0,0,0));
    case 1: return QDateTime(QDate(1900, 1, 2), QTime(1,1,1,1));
    case 2: return QDateTime(QDate(1900, 1, 3), QTime(12,0,0,0));
    case 3: return QDateTime(QDate(1900, 1, 4), QTime(23,59,59,999));
    case 4: return QDateTime(QDate(1999, 1, 1), QTime(0,0,0,0));
    case 5: return QDateTime(QDate(1999, 1, 2), QTime(1,1,1,1));
    case 6: return QDateTime(QDate(1999, 1, 3), QTime(12,0,0,0));
    case 7: return QDateTime(QDate(1999, 1, 4), QTime(23,59,59,999));
    case 8: return QDateTime(QDate(2000, 1, 1), QTime(0,0,0,0));
    case 9: return QDateTime(QDate(2000, 1, 2), QTime(1,1,1,1));
    case 10: return QDateTime(QDate(2000, 1, 3), QTime(12,0,0,0));
    case 11: return QDateTime(QDate(2000, 1, 4), QTime(23,59,59,999));
    case 12: return QDateTime(QDate(2000, 12, 31), QTime(0,0,0,0));
    case 13: return QDateTime(QDate(2000, 12, 31), QTime(1,1,1,1));
    case 14: return QDateTime(QDate(2000, 12, 31), QTime(12,0,0,0));
    case 15: return QDateTime(QDate(2000, 12, 31), QTime(23,59,59,999));
    }
    return QDateTime(QDate(1900, 1, 1), QTime(0,0,0));
}
#define MAX_QDATETIME_DATA 16

void tst_QDataStream::stream_QDateTime_data()
{
    stream_data(MAX_QDATETIME_DATA);
}

void tst_QDataStream::stream_QDateTime()
{
    STREAM_IMPL(QDateTime);
}

void tst_QDataStream::writeQDateTime(QDataStream *s)
{
    QDateTime dt(qDateTimeData(dataIndex(QTest::currentDataTag())));
    *s << dt;
}

void tst_QDataStream::readQDateTime(QDataStream *s)
{
    QDateTime test(qDateTimeData(dataIndex(QTest::currentDataTag())));
    QDateTime d8;
    *s >> d8;
    QVERIFY(d8 == test);
}

// ************************************

static QFont qFontData(int index)
{
    switch (index) {
    case 0: return QFont("Courier", 20, QFont::Bold, true);
    case 1: return QFont("Courier", 18, QFont::Bold, false);
    case 2: return QFont("Courier", 16, QFont::Light, true);
    case 3: return QFont("Courier", 14, QFont::Normal, false);
    case 4: return QFont("Courier", 12, QFont::DemiBold, true);
    case 5: return QFont("Courier", 10, QFont::Black, false);
    case 6:
        {
            QFont f("Helvetica", 10, QFont::Normal, false);
            f.setPixelSize(2);
            f.setUnderline(false);
            f.setStrikeOut(false);
            f.setFixedPitch(false);
            return f;
        }
    case 7:
        {
            QFont f("Helvetica", 10, QFont::Bold, false);
            f.setPixelSize(4);
            f.setUnderline(true);
            f.setStrikeOut(false);
            f.setFixedPitch(false);
            return f;
        }
    case 8:
        {
            QFont f("Helvetica", 10, QFont::Light, false);
            f.setPixelSize(6);
            f.setUnderline(false);
            f.setStrikeOut(true);
            f.setFixedPitch(false);
            return f;
        }
    case 9:
        {
            QFont f("Helvetica", 10, QFont::DemiBold, false);
            f.setPixelSize(8);
            f.setUnderline(false);
            f.setStrikeOut(false);
            f.setFixedPitch(true);
            return f;
        }
    case 10:
        {
            QFont f("Helvetica", 10, QFont::Black, false);
            f.setPixelSize(10);
            f.setUnderline(true);
            f.setStrikeOut(true);
            f.setFixedPitch(false);
            return f;
        }
    case 11:
        {
            QFont f("Helvetica", 10, QFont::Normal, true);
            f.setPixelSize(12);
            f.setUnderline(false);
            f.setStrikeOut(true);
            f.setFixedPitch(true);
            return f;
        }
    case 12:
        {
            QFont f("Helvetica", 10, QFont::Bold, true);
            f.setPixelSize(14);
            f.setUnderline(true);
            f.setStrikeOut(true);
            f.setFixedPitch(true);
            return f;
        }
    case 13:
        {
            QFont f("Helvetica", 10, QFont::Bold, true);
            f.setStretch(200);
            return f;
        }
    }
    return QFont("Courier", 18, QFont::Bold, true);
}
#define MAX_QFONT_DATA 14

void tst_QDataStream::stream_QFont_data()
{
    stream_data(MAX_QFONT_DATA);
}

void tst_QDataStream::stream_QFont()
{
    STREAM_IMPL(QFont);
}

void tst_QDataStream::writeQFont(QDataStream *s)
{
    QFont d9(qFontData(dataIndex(QTest::currentDataTag())));
    *s << d9;
}

void tst_QDataStream::readQFont(QDataStream *s)
{
    QFont test(qFontData(dataIndex(QTest::currentDataTag())));
    QFont d9;
    *s >> d9;

    // maybe a bit overkill ...
    QCOMPARE(d9.family(), test.family());
    QCOMPARE(d9.pointSize(), test.pointSize());
    QCOMPARE(d9.pixelSize(), test.pixelSize());
    QCOMPARE(d9.weight(), test.weight());
    QCOMPARE(d9.bold(), test.bold());
    QCOMPARE(d9.italic(), test.italic());
    QCOMPARE(d9.underline(), test.underline());
    QCOMPARE(d9.overline(), test.overline());
    QCOMPARE(d9.strikeOut(), test.strikeOut());
    QCOMPARE(d9.fixedPitch(), test.fixedPitch());
    QCOMPARE(d9.styleHint(), test.styleHint());
    QCOMPARE(d9.toString(), test.toString());

    QCOMPARE(d9, test);
}

// ************************************

void tst_QDataStream::stream_QImage_data()
{
    stream_data(1);
}

void tst_QDataStream::stream_QImage()
{
    STREAM_IMPL(QImage);
}

void tst_QDataStream::writeQImage(QDataStream *s)
{
    QImage d12(open_xpm);
    *s << d12;
}

void tst_QDataStream::readQImage(QDataStream *s)
{
    QImage ref(open_xpm);

    QImage d12;
    *s >> d12;
    QVERIFY(d12 == ref);

    // do some extra neurotic tests
    QVERIFY(d12.size() == ref.size());
    QVERIFY(d12.isNull() == ref.isNull());
    QVERIFY(d12.width() == ref.width());
    QVERIFY(d12.height() == ref.height());
    QVERIFY(d12.depth() == ref.depth());
    QVERIFY(d12.colorCount() == ref.colorCount());
    QVERIFY(d12.hasAlphaChannel() == ref.hasAlphaChannel());
}

// ************************************

static QPen qPenData(int index)
{
    switch (index) {
    case 0:
        {
            QPen p(Qt::blue, 0, Qt::NoPen);
            p.setCapStyle(Qt::FlatCap);
            p.setJoinStyle(Qt::MiterJoin);
            return p;
        }
    case 1:
        {
            QPen p(Qt::red, 1, Qt::SolidLine);
            p.setCapStyle(Qt::SquareCap);
            p.setJoinStyle(Qt::BevelJoin);
            return p;
        }
    case 2:
        {
            QPen p(Qt::red, 4, Qt::DashDotDotLine);
            p.setCapStyle(Qt::RoundCap);
            p.setJoinStyle(Qt::RoundJoin);
            return p;
        }
    case 3:
        {
            QPen p(Qt::blue, 12, Qt::NoPen);
            p.setCapStyle(Qt::FlatCap);
            p.setJoinStyle(Qt::RoundJoin);
            return p;
        }
    case 4:
        {
            QPen p(Qt::red, 99, Qt::SolidLine);
            p.setCapStyle(Qt::SquareCap);
            p.setJoinStyle(Qt::MiterJoin);
            return p;
        }
    case 5:
        {
            QPen p(Qt::red, 255, Qt::DashDotLine);
            p.setCapStyle(Qt::RoundCap);
            p.setJoinStyle(Qt::BevelJoin);
            return p;
        }
    case 6:
        {
            QPen p(Qt::red, 256, Qt::DashDotLine);
            p.setCapStyle(Qt::RoundCap);
            p.setJoinStyle(Qt::BevelJoin);
            return p;
        }
    case 7:
        {
            QPen p(Qt::red, 0.25, Qt::DashDotLine);
            p.setCapStyle(Qt::RoundCap);
            p.setJoinStyle(Qt::BevelJoin);
            return p;
        }
    }

    return QPen();
}
#define MAX_QPEN_DATA 8

void tst_QDataStream::stream_QPen_data()
{
    stream_data(MAX_QPEN_DATA);
}

void tst_QDataStream::stream_QPen()
{
    STREAM_IMPL(QPen);
}

void tst_QDataStream::writeQPen(QDataStream *s)
{
    QPen d15(qPenData(dataIndex(QTest::currentDataTag())));
    *s << d15;
}

void tst_QDataStream::readQPen(QDataStream *s)
{
    QPen origPen(qPenData(dataIndex(QTest::currentDataTag())));
    QPen d15;
    *s >> d15;
    QCOMPARE(d15.style(), origPen.style());
    QCOMPARE(d15.width(), origPen.width());
    QCOMPARE(d15.color(), origPen.color());
    QVERIFY(d15.capStyle() == origPen.capStyle());
    QVERIFY(d15.joinStyle() == origPen.joinStyle());
    QVERIFY(d15 == origPen);
}

// ************************************

// pixmap testing is currently limited to one pixmap only.
//
#ifndef Q_OS_WINCE
// Test depends on more memory than available on Qt/CE
void tst_QDataStream::stream_QPixmap_data()
{
    stream_data(1);
}
#endif

#ifndef Q_OS_WINCE
// Test depends on more memory than available on Qt/CE
void tst_QDataStream::stream_QPixmap()
{
    STREAM_IMPL(QPixmap);
}
#endif

#ifndef Q_OS_WINCE
// Test depends on more memory than available on Qt/CE
void tst_QDataStream::stream_QIcon_data()
{
    stream_data(1);
}

void tst_QDataStream::stream_QIcon()
{
    STREAM_IMPL(QIcon);
}
#endif

void tst_QDataStream::writeQPixmap(QDataStream *s)
{
    QPixmap d16(open_xpm);
    *s << d16;
}

void tst_QDataStream::readQPixmap(QDataStream *s)
{
    QPixmap pm(open_xpm);
    QPixmap d16;
    *s >> d16;
    QVERIFY(!d16.isNull() && !pm.isNull());
    QVERIFY(d16.width() == pm.width());
    QVERIFY(d16.height() == pm.height());
    QVERIFY(d16.size() == pm.size());
    QVERIFY(d16.rect() == pm.rect());
    QVERIFY(d16.depth() == pm.depth());
}

void tst_QDataStream::writeQIcon(QDataStream *s)
{
    QPixmap pm(open_xpm);
    QIcon d16(pm);
    *s << d16;
}

void tst_QDataStream::readQIcon(QDataStream *s)
{
    QPixmap pm(open_xpm);
    QIcon icon(pm);
    QIcon d16;
    *s >> d16;
    QVERIFY(!d16.isNull() && !icon.isNull());
    QCOMPARE(d16.pixmap(100), pm);
}

// ************************************

QPoint qPointData(int index)
{
    switch (index) {
    case 0: return QPoint(0, 0);
    case 1: return QPoint(-1, 0);
    case 2: return QPoint(0, -1);
    case 3: return QPoint(1, 0);
    case 4: return QPoint(0, 1);
    case 5: return QPoint(-1, -1);
    case 6: return QPoint(1, 1);
    case 7: return QPoint(255, 255);
    case 8: return QPoint(256, 256);
    case 9: return QPoint(-254, -254);
    case 10: return QPoint(-255, -255);
    }

    return QPoint();
}
#define MAX_QPOINT_DATA 11


void tst_QDataStream::stream_QPoint_data()
{
    stream_data(MAX_QPOINT_DATA);
}

void tst_QDataStream::stream_QPoint()
{
    STREAM_IMPL(QPoint);
}

void tst_QDataStream::writeQPoint(QDataStream *s)
{
    QPoint d17(qPointData(dataIndex(QTest::currentDataTag())));
    *s << d17;

    QPointF d17f = d17;
    *s << d17f;
}

void tst_QDataStream::readQPoint(QDataStream *s)
{
    QPoint ref(qPointData(dataIndex(QTest::currentDataTag())));
    QPoint d17;
    *s >> d17;
    QVERIFY(d17 == ref);

    QPointF d17f;
    *s >> d17f;
    QVERIFY(d17f == QPointF(ref));
}

// ************************************

static QRect qRectData(int index)
{
    switch (index) {
    case 0: return QRect(0, 0, 0, 0);
    case 1: return QRect(1, 1, 1, 1);
    case 2: return QRect(1, 2, 3, 4);
    case 3: return QRect(-1, -1, -1, -1);
    case 4: return QRect(-1, -2, -3, -4);
    case 5: return QRect(255, -5, 256, -6);
    case 6: return QRect(-7, 255, -8, 256);
    case 7: return QRect(9, -255, 10, -255);
    case 8: return QRect(-255, 11, -255, 12);
    case 9: return QRect(256, 512, 1024, 2048);
    case 10: return QRect(-256, -512, -1024, -2048);
    }
    return QRect();
}
#define MAX_QRECT_DATA 11

void tst_QDataStream::stream_QRect_data()
{
    stream_data(MAX_QRECT_DATA);
}

void tst_QDataStream::stream_QRect()
{
    STREAM_IMPL(QRect);
}

void tst_QDataStream::writeQRect(QDataStream *s)
{
    QRect d18(qRectData(dataIndex(QTest::currentDataTag())));
    *s << d18;

    QRectF d18f(d18);
    *s << d18f;
}

void tst_QDataStream::readQRect(QDataStream *s)
{
    QRect ref(qRectData(dataIndex(QTest::currentDataTag())));
    QRect d18;
    *s >> d18;
    QVERIFY(d18 == ref);

    QRectF d18f;
    *s >> d18f;
    QVERIFY(d18f == QRectF(ref));
}

// ************************************

static QPolygon qPolygonData(int index)
{
    QPoint p0(0, 0);
    QPoint p1(1, 1);
    QPoint p2(-1, -1);
    QPoint p3(1, -1);
    QPoint p4(-1, 1);
    QPoint p5(0, 255);
    QPoint p6(0, 256);
    QPoint p7(0, 1024);
    QPoint p8(255, 0);
    QPoint p9(256, 0);
    QPoint p10(1024, 0);
    QPoint p11(345, 678);
    QPoint p12(23456, 99999);
    QPoint p13(-99998, -34567);
    QPoint p14(45678, -99999);

    switch (index) {
    case 0:
        return QPolygon(0);
    case 1:
        {
            QPolygon p(1);
            p.setPoint(0, p0);
            return p;
        }
    case 2:
        {
            QPolygon p(1);
            p.setPoint(0, p5);
            return p;
        }
    case 3:
        {
            QPolygon p(1);
            p.setPoint(0, p12);
            return p;
        }
    case 4:
        {
            QPolygon p(3);
            p.setPoint(0, p1);
            p.setPoint(1, p10);
            p.setPoint(2, p13);
            return p;
        }
    case 5:
        {
            QPolygon p(6);
            p.setPoint(0, p2);
            p.setPoint(1, p11);
            p.setPoint(2, p14);
            return p;
        }
    case 6:
        {
            QPolygon p(15);
            p.setPoint(0, p0);
            p.setPoint(1, p1);
            p.setPoint(2, p2);
            p.setPoint(3, p3);
            p.setPoint(4, p4);
            p.setPoint(5, p5);
            p.setPoint(6, p6);
            p.setPoint(7, p7);
            p.setPoint(8, p8);
            p.setPoint(9, p9);
            p.setPoint(10, p10);
            p.setPoint(11, p11);
            p.setPoint(12, p12);
            p.setPoint(13, p13);
            p.setPoint(14, p14);
            return p;
        }
    }
    return QRect();
}
#define MAX_QPOINTARRAY_DATA 7

void tst_QDataStream::stream_QPolygon_data()
{
    stream_data(1);
}

void tst_QDataStream::stream_QPolygon()
{
    STREAM_IMPL(QPolygon);
}

void tst_QDataStream::writeQPolygon(QDataStream *s)
{
    QPolygon d19(qPolygonData(dataIndex(QTest::currentDataTag())));
    *s << d19;

    QPolygonF d19f(d19);
    *s << d19f;
}

void tst_QDataStream::readQPolygon(QDataStream *s)
{
    QPolygon ref(qPolygonData(dataIndex(QTest::currentDataTag())));
    QPolygon d19;
    *s >> d19;
    QVERIFY(d19 == ref);

    QPolygonF d19f;
    *s >> d19f;
    QVERIFY(d19f == QPolygonF(ref));
}

// ************************************

static QRegion qRegionData(int index)
{
    switch (index) {
    case 0: return QRegion(0, 0, 0, 0, QRegion::Rectangle);
    case 1:
        {
            QRegion r(1, 2, 300, 400, QRegion::Rectangle);
            if (r != QRegion(1, 2, 300, 400, QRegion::Rectangle))
                qDebug("Error creating a region");
            return r;
        }
    case 2: return QRegion(100, 100, 1024, 768, QRegion::Rectangle);
    case 3: return QRegion(-100, -100, 1024, 1024, QRegion::Rectangle);
    case 4: return QRegion(100, -100, 2048, 4096, QRegion::Rectangle);
    case 5: return QRegion(-100, 100, 4096, 2048, QRegion::Rectangle);
    case 6: return QRegion(0, 0, 0, 0, QRegion::Ellipse);
#if (!defined(Q_OS_UNIX) && !defined(Q_OS_WINCE)) // all our Unix platforms use X regions.
    case 7: return QRegion(1, 2, 300, 400, QRegion::Ellipse);
    case 8: return QRegion(100, 100, 1024, 768, QRegion::Ellipse);
    case 9: return QRegion(-100, -100, 1024, 1024, QRegion::Ellipse);
    case 10: return QRegion(100, -100, 2048, 4096, QRegion::Ellipse);
    case 11: return QRegion(-100, 100, 4096, 2048, QRegion::Ellipse);
        // simplest X11 case that fails:
    case 12: return QRegion(0, 0, 3, 3, QRegion::Ellipse);
#else
    case 7:
        qWarning("Skipping streaming of elliptical regions on embedded, Mac OS X, and X11;"
                 " our pointarray stuff is not that great at approximating.");
#endif
    }
    return QRegion();
}
#define MAX_QREGION_DATA 12

void tst_QDataStream::stream_QRegion_data()
{
    stream_data(MAX_QREGION_DATA);
}

void tst_QDataStream::stream_QRegion()
{
    STREAM_IMPL(QRegion);
}

void tst_QDataStream::writeQRegion(QDataStream *s)
{
    QRegion r(qRegionData(dataIndex(QTest::currentDataTag())));
    *s << r;
}

void tst_QDataStream::readQRegion(QDataStream *s)
{
    QRegion ref(qRegionData(dataIndex(QTest::currentDataTag())));
    QRegion r;
    *s >> r;
    QVERIFY(r == ref);
}

// ************************************

static QSize qSizeData(int index)
{
    switch (index) {
    case 0: return QSize(0, 0);
    case 1: return QSize(-1, 0);
    case 2: return QSize(0, -1);
    case 3: return QSize(1, 0);
    case 4: return QSize(0, 1);
    case 5: return QSize(-1, -1);
    case 6: return QSize(1, 1);
    case 7: return QSize(255, 255);
    case 8: return QSize(256, 256);
    case 9: return QSize(-254, -254);
    case 10: return QSize(-255, -255);
    }
    return QSize();
}
#define MAX_QSIZE_DATA 11

void tst_QDataStream::stream_QSize_data()
{
    stream_data(MAX_QSIZE_DATA);
}

void tst_QDataStream::stream_QSize()
{
    STREAM_IMPL(QSize);
}

void tst_QDataStream::writeQSize(QDataStream *s)
{
    QSize d21(qSizeData(dataIndex(QTest::currentDataTag())));
    *s << d21;

    QSizeF d21f(d21);
    *s << d21f;
}

void tst_QDataStream::readQSize(QDataStream *s)
{
    QSize ref(qSizeData(dataIndex(QTest::currentDataTag())));
    QSize d21;
    *s >> d21;
    QVERIFY(d21 == ref);

    QSizeF d21f;
    *s >> d21f;
    QVERIFY(d21f == QSizeF(ref));
}

// *********************** atEnd ******************************

void tst_QDataStream::stream_atEnd_data()
{
    stream_data(MAX_QSTRING_DATA);
}

void tst_QDataStream::stream_atEnd()
{
    QFETCH(QString, device);
    if (device == "bytearray") {
        QByteArray ba;
        QDataStream sout(&ba, QIODevice::WriteOnly);
        writeQString(&sout);

        QDataStream sin(&ba, QIODevice::ReadOnly);
        readQString(&sin);
        QVERIFY(sin.atEnd());
    } else if (device == "file") {
        QString fileName = "qdatastream.out";
        QFile fOut(fileName);
        QVERIFY(fOut.open(QIODevice::WriteOnly));
        QDataStream sout(&fOut);
        writeQString(&sout);
        fOut.close();

        QFile fIn(fileName);
        QVERIFY(fIn.open(QIODevice::ReadOnly));
        QDataStream sin(&fIn);
        readQString(&sin);
        QVERIFY(sin.atEnd());
        fIn.close();
    } else if (device == "buffer") {
        {
            QByteArray ba(0);
            QBuffer bOut(&ba);
            bOut.open(QIODevice::WriteOnly);
            QDataStream sout(&bOut);
            writeQString(&sout);
            bOut.close();

            QBuffer bIn(&ba);
            bIn.open(QIODevice::ReadOnly);
            QDataStream sin(&bIn);
            readQString(&sin);
            QVERIFY(sin.atEnd());
            bIn.close();
        }

        // Do the same test again, but this time with an initial size for the bytearray.
        {
            QByteArray ba(10000, '\0');
            QBuffer bOut(&ba);
            bOut.open(QIODevice::WriteOnly | QIODevice::Truncate);
            QDataStream sout(&bOut);
            writeQString(&sout);
            bOut.close();

            QBuffer bIn(&ba);
            bIn.open(QIODevice::ReadOnly);
            QDataStream sin(&bIn);
            readQString(&sin);
            QVERIFY(sin.atEnd());
            bIn.close();
        }
    }
}

class FakeBuffer : public QBuffer
{
protected:
    qint64 writeData(const char *c, qint64 i) { return m_lock ? 0 : QBuffer::writeData(c, i); }
public:
    FakeBuffer(bool locked = false) : m_lock(locked) {}
    void setLocked(bool locked) { m_lock = locked; }
private:
    bool m_lock;
};

#define TEST_WRITE_ERROR(op) \
    { \
        FakeBuffer fb(false); \
        QVERIFY(fb.open(QBuffer::ReadWrite)); \
        QDataStream fs(&fb); \
        fs.writeRawData("hello", 5); \
        /* first write some initial content */ \
        QCOMPARE(fs.status(), QDataStream::Ok); \
        QCOMPARE(fb.data(), QByteArray("hello")); \
        /* then test that writing can cause an error */ \
        fb.setLocked(true); \
        fs op; \
        QCOMPARE(fs.status(), QDataStream::WriteFailed); \
        QCOMPARE(fb.data(), QByteArray("hello")); \
        /* finally test that writing after an error doesn't change the stream any more */ \
        fb.setLocked(false); \
        fs op; \
        QCOMPARE(fs.status(), QDataStream::WriteFailed); \
        QCOMPARE(fb.data(), QByteArray("hello")); \
    }

void tst_QDataStream::stream_writeError()
{
    TEST_WRITE_ERROR(<< true)
    TEST_WRITE_ERROR(<< (qint8)1)
    TEST_WRITE_ERROR(<< (quint8)1)
    TEST_WRITE_ERROR(<< (qint16)1)
    TEST_WRITE_ERROR(<< (quint16)1)
    TEST_WRITE_ERROR(<< (qint32)1)
    TEST_WRITE_ERROR(<< (quint32)1)
    TEST_WRITE_ERROR(<< (qint64)1)
    TEST_WRITE_ERROR(<< (quint64)1)
    TEST_WRITE_ERROR(<< "hello")
    TEST_WRITE_ERROR(<< (float)1.0)
    TEST_WRITE_ERROR(<< (double)1.0)
    TEST_WRITE_ERROR(.writeRawData("test", 4))
}

void tst_QDataStream::stream_QByteArray2()
{
    QByteArray ba;
    {
        QDataStream s(&ba, QIODevice::WriteOnly);
        s << QByteArray("hallo");
        s << QByteArray("");
        s << QByteArray();
    }

    {
        QDataStream s(&ba, QIODevice::ReadOnly);
        QByteArray res;
        s >> res;
        QCOMPARE(res, QByteArray("hallo"));
        s >> res;
        QCOMPARE(res, QByteArray(""));
        QVERIFY(res.isEmpty());
        QVERIFY(!res.isNull());
        s >> res;
        QCOMPARE(res, QByteArray());
        QVERIFY(res.isEmpty());
        QVERIFY(res.isNull());
    }
}

void tst_QDataStream::setVersion_data()
{
    QTest::addColumn<int>("vers");
    QDataStream latest;

    for (int vers = 1; vers <= latest.version(); ++vers)
        QTest::newRow(qPrintable(QString("v_%1").arg(vers))) << vers;
}

void tst_QDataStream::setVersion()
{
    QDataStream latest;
    QFETCH(int, vers);

    /*
        Test QKeySequence.
    */
    QByteArray ba1;
    {
        QDataStream out(&ba1, QIODevice::WriteOnly);
        out.setVersion(vers);
        out << QKeySequence(Qt::Key_A) << QKeySequence(Qt::Key_B, Qt::Key_C)
                << (quint32)0xDEADBEEF;
    }
    {
        QKeySequence keyseq1, keyseq2;
        quint32 deadbeef;
        QDataStream in(&ba1, QIODevice::ReadOnly);
        in.setVersion(vers);
        in >> keyseq1 >> keyseq2 >> deadbeef;
        QVERIFY(keyseq1 == QKeySequence(Qt::Key_A));
        if (vers >= 5) {
            QVERIFY(keyseq2 == QKeySequence(Qt::Key_B, Qt::Key_C));
        } else {
            QVERIFY(keyseq2 == QKeySequence(Qt::Key_B));
        }
        QVERIFY(deadbeef == 0xDEADBEEF);
    }

    /*
        Test QPalette.
    */

    // revise the test if new color roles or color groups are added
    QVERIFY(QPalette::NColorRoles == QPalette::ToolTipText + 1);
    QVERIFY(QPalette::NColorGroups == 3);

    QByteArray ba2;
    QPalette pal1, pal2;
    for (int grp = 0; grp < (int)QPalette::NColorGroups; ++grp) {
        for (int role = 0; role < (int)QPalette::NColorRoles; ++role) {
            // random stuff
            pal1.setColor((QPalette::ColorGroup)grp, (QPalette::ColorRole)role,
                           QColor(grp * 13, 255 - grp, role));
            pal2.setColor((QPalette::ColorGroup)grp, (QPalette::ColorRole)role,
                           QColor(role * 11, 254 - role, grp));
        }
    }

    {
        QDataStream out(&ba2, QIODevice::WriteOnly);
        out.setVersion(vers);
        out << pal1 << pal2 << (quint32)0xCAFEBABE;
    }
    {
        QPalette inPal1, inPal2;
        quint32 cafebabe;
        QDataStream in(&ba2, QIODevice::ReadOnly);
        in.setVersion(vers);
        in >> inPal1 >> inPal2;
        in >> cafebabe;

        QCOMPARE(cafebabe, 0xCAFEBABE);

        QCOMPARE(NColorRoles[latest.version()], (int)QPalette::NColorRoles);  //if this fails you need to update the NColorRoles  array

        if (vers == 1) {
            for (int grp = 0; grp < (int)QPalette::NColorGroups; ++grp) {
                QVERIFY(pal1.color((QPalette::ColorGroup)grp, QPalette::Foreground)
                        == inPal1.color((QPalette::ColorGroup)grp, QPalette::Foreground));
                QVERIFY(pal1.color((QPalette::ColorGroup)grp, QPalette::Background)
                        == inPal1.color((QPalette::ColorGroup)grp, QPalette::Background));
                QVERIFY(pal1.color((QPalette::ColorGroup)grp, QPalette::Light)
                        == inPal1.color((QPalette::ColorGroup)grp, QPalette::Light));
                QVERIFY(pal1.color((QPalette::ColorGroup)grp, QPalette::Dark)
                        == inPal1.color((QPalette::ColorGroup)grp, QPalette::Dark));
                QVERIFY(pal1.color((QPalette::ColorGroup)grp, QPalette::Mid)
                        == inPal1.color((QPalette::ColorGroup)grp, QPalette::Mid));
                QVERIFY(pal1.color((QPalette::ColorGroup)grp, QPalette::Text)
                        == inPal1.color((QPalette::ColorGroup)grp, QPalette::Text));
                QVERIFY(pal1.color((QPalette::ColorGroup)grp, QPalette::Base)
                        == inPal1.color((QPalette::ColorGroup)grp, QPalette::Base));

                QVERIFY(pal1.color((QPalette::ColorGroup)grp, QPalette::Midlight)
                        != inPal1.color((QPalette::ColorGroup)grp, QPalette::Midlight));
            }
        } else {
            if (NColorRoles[vers] < QPalette::NColorRoles) {
                QVERIFY(pal1 != inPal1);
                QVERIFY(pal2 != inPal2);

                for (int grp = 0; grp < (int)QPalette::NColorGroups; ++grp) {
                    for (int i = NColorRoles[vers]; i < QPalette::NColorRoles; ++i) {
                        inPal1.setColor((QPalette::ColorGroup)grp, (QPalette::ColorRole)i,
                                         pal1.color((QPalette::ColorGroup)grp, (QPalette::ColorRole)i));
                        inPal2.setColor((QPalette::ColorGroup)grp, (QPalette::ColorRole)i,
                                         pal2.color((QPalette::ColorGroup)grp, (QPalette::ColorRole)i));
                    }
                }
            }
            QVERIFY(pal1 == inPal1);
            QVERIFY(pal2 == inPal2);
        }
    }
}

class SequentialBuffer : public QBuffer
{
public:
    SequentialBuffer(QByteArray *data) : QBuffer(data) { offset = 0; }

    bool isSequential() const { return true; }
    bool seek(qint64 pos) { offset = pos; return QBuffer::seek(pos); }
    qint64 pos() const { return qint64(offset); }

protected:
    qint64 readData(char *data, qint64 maxSize)
    {
        qint64 ret = QBuffer::readData(data, maxSize);
        offset += ret;
        return ret;
    }

private:
    int offset;
};

void tst_QDataStream::skipRawData_data()
{
    QTest::addColumn<QString>("deviceType");
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<int>("read");
    QTest::addColumn<int>("skip");
    QTest::addColumn<int>("skipped");
    QTest::addColumn<char>("expect");

    QByteArray bigData;
    bigData.fill('a', 20000);
    bigData[10001] = 'x';

    QTest::newRow("1") << QString("sequential")    << QByteArray("abcdefghij") << 3 << 6 << 6 << 'j';
    QTest::newRow("2") << QString("random-access") << QByteArray("abcdefghij") << 3 << 6 << 6 << 'j';
    QTest::newRow("3") << QString("sequential")    << bigData << 1 << 10000 << 10000 << 'x';
    QTest::newRow("4") << QString("random-access") << bigData << 1 << 10000 << 10000 << 'x';
    QTest::newRow("5") << QString("sequential")    << bigData << 1 << 20000 << 19999 << '\0';
    QTest::newRow("6") << QString("random-access") << bigData << 1 << 20000 << 19999 << '\0';
}

void tst_QDataStream::skipRawData()
{
    QFETCH(QString, deviceType);
    QFETCH(QByteArray, data);
    QFETCH(int, read);
    QFETCH(int, skip);
    QFETCH(int, skipped);
    QFETCH(char, expect);
    qint8 dummy;

    QIODevice *dev = 0;
    if (deviceType == "sequential") {
        dev = new SequentialBuffer(&data);
    } else if (deviceType == "random-access") {
        dev = new QBuffer(&data);
    }
    QVERIFY(dev);
    dev->open(QIODevice::ReadOnly);

    QDataStream in(dev);
    for (int i = 0; i < read; ++i)
        in >> dummy;

    QCOMPARE(in.skipRawData(skip), skipped);
    in >> dummy;
    QCOMPARE((char)dummy, expect);

    delete dev;
}

#define TEST_qint(T, UT) \
    void tst_QDataStream::status_##T() \
    { \
        QFETCH(QByteArray, bigEndianData); \
        QFETCH(QByteArray, littleEndianData); \
        QFETCH(int, expectedStatus); \
        QFETCH(qint64, expectedValue); \
    \
        { \
            QDataStream stream(&bigEndianData, QIODevice::ReadOnly); \
            T i; \
            stream >> i; \
            QCOMPARE((int) stream.status(), expectedStatus); \
            QCOMPARE(i, (T) expectedValue); \
        } \
        { \
            QDataStream stream(&bigEndianData, QIODevice::ReadOnly); \
            UT i; \
            stream >> i; \
            QCOMPARE((int) stream.status(), expectedStatus); \
            QCOMPARE((T) i, (T) expectedValue); \
        } \
        { \
            QDataStream stream(&littleEndianData, QIODevice::ReadOnly); \
            stream.setByteOrder(QDataStream::LittleEndian); \
            T i; \
            stream >> i; \
            QCOMPARE((int) stream.status(), expectedStatus); \
            QCOMPARE(i, (T) expectedValue); \
        } \
        { \
            QDataStream stream(&littleEndianData, QIODevice::ReadOnly); \
            stream.setByteOrder(QDataStream::LittleEndian); \
            UT i; \
            stream >> i; \
            QCOMPARE((int) stream.status(), expectedStatus); \
            QCOMPARE((T) i, (T) expectedValue); \
        } \
    }

#define TEST_FLOAT(T) \
    void tst_QDataStream::status_##T() \
    { \
        QFETCH(QByteArray, bigEndianData); \
        QFETCH(QByteArray, littleEndianData); \
        QFETCH(int, expectedStatus); \
        QFETCH(double, expectedValue); \
        \
        QDataStream::FloatingPointPrecision prec = sizeof(T) == sizeof(double) ? QDataStream::DoublePrecision : QDataStream::SinglePrecision; \
    \
        { \
            QDataStream stream(&bigEndianData, QIODevice::ReadOnly); \
            stream.setFloatingPointPrecision(prec); \
            T i; \
            stream >> i; \
            QCOMPARE((int) stream.status(), expectedStatus); \
            QCOMPARE((float) i, (float) expectedValue); \
        } \
        { \
            QDataStream stream(&littleEndianData, QIODevice::ReadOnly); \
            stream.setByteOrder(QDataStream::LittleEndian); \
            stream.setFloatingPointPrecision(prec); \
            T i; \
            stream >> i; \
            QCOMPARE((int) stream.status(), expectedStatus); \
            QCOMPARE((float) i, (float) expectedValue); \
        } \
    }

void tst_QDataStream::status_qint8_data()
{
    QTest::addColumn<QByteArray>("bigEndianData");
    QTest::addColumn<QByteArray>("littleEndianData");
    QTest::addColumn<int>("expectedStatus");
    QTest::addColumn<qint64>("expectedValue");

    // ok
    QTest::newRow("0") << QByteArray(1, '\x0') << QByteArray(1, '\x0') << (int) QDataStream::Ok << qint64(0);
    QTest::newRow("-1") << QByteArray(1, '\xff') << QByteArray(1, '\xff') << (int) QDataStream::Ok << qint64(-1);
    QTest::newRow("1") << QByteArray(1, '\x01') << QByteArray(1, '\x01') << (int) QDataStream::Ok << qint64(1);
    QTest::newRow("37") << QByteArray(1, '\x25') << QByteArray(1, '\x25') << (int) QDataStream::Ok << qint64(37);
    QTest::newRow("37j") << QByteArray("\x25j") << QByteArray("\x25j") << (int) QDataStream::Ok << qint64(37);

    // past end
    QTest::newRow("empty") << QByteArray() << QByteArray() << (int) QDataStream::ReadPastEnd << qint64(0);
}

TEST_qint(qint8, quint8)

void tst_QDataStream::status_qint16_data()
{
    QTest::addColumn<QByteArray>("bigEndianData");
    QTest::addColumn<QByteArray>("littleEndianData");
    QTest::addColumn<int>("expectedStatus");
    QTest::addColumn<qint64>("expectedValue");

    // ok
    QTest::newRow("0") << QByteArray(2, '\x0') << QByteArray(2, '\x0') << (int) QDataStream::Ok << qint64(0);
    QTest::newRow("-1") << QByteArray("\xff\xff", 2) << QByteArray("\xff\xff", 2) << (int) QDataStream::Ok << qint64(-1);
    QTest::newRow("1") << QByteArray("\x00\x01", 2) << QByteArray("\x01\x00", 2) << (int) QDataStream::Ok << qint64(1);
    QTest::newRow("37") << QByteArray("\x00\x25", 2) << QByteArray("\x25\x00", 2) << (int) QDataStream::Ok << qint64(37);
    QTest::newRow("37j") << QByteArray("\x00\x25j", 3) << QByteArray("\x25\x00j", 3) << (int) QDataStream::Ok << qint64(37);
    QTest::newRow("0x1234") << QByteArray("\x12\x34", 2) << QByteArray("\x34\x12", 2) << (int) QDataStream::Ok << qint64(0x1234);

    // past end
    QTest::newRow("empty") << QByteArray() << QByteArray() << (int) QDataStream::ReadPastEnd << qint64(0);
    QTest::newRow("end 1") << QByteArray("", 1) << QByteArray("", 1) << (int) QDataStream::ReadPastEnd << qint64(0);
    QTest::newRow("end 2") << QByteArray("\x25", 1) << QByteArray("\x25", 1) << (int) QDataStream::ReadPastEnd << qint64(0);
}

TEST_qint(qint16, quint16)

void tst_QDataStream::status_qint32_data()
{
    QTest::addColumn<QByteArray>("bigEndianData");
    QTest::addColumn<QByteArray>("littleEndianData");
    QTest::addColumn<int>("expectedStatus");
    QTest::addColumn<qint64>("expectedValue");

    // ok
    QTest::newRow("0") << QByteArray(4, '\x0') << QByteArray(4, '\x0') << (int) QDataStream::Ok << qint64(0);
    QTest::newRow("-1") << QByteArray("\xff\xff\xff\xff", 4) << QByteArray("\xff\xff\xff\xff", 4) << (int) QDataStream::Ok << qint64(-1);
    QTest::newRow("1") << QByteArray("\x00\x00\x00\x01", 4) << QByteArray("\x01\x00\x00\x00", 4) << (int) QDataStream::Ok << qint64(1);
    QTest::newRow("37") << QByteArray("\x00\x00\x00\x25", 4) << QByteArray("\x25\x00\x00\x00", 4) << (int) QDataStream::Ok << qint64(37);
    QTest::newRow("37j") << QByteArray("\x00\x00\x00\x25j", 5) << QByteArray("\x25\x00\x00\x00j", 5) << (int) QDataStream::Ok << qint64(37);
    QTest::newRow("0x12345678") << QByteArray("\x12\x34\x56\x78", 4) << QByteArray("\x78\x56\x34\x12", 4) << (int) QDataStream::Ok << qint64(0x12345678);

    // past end
    QTest::newRow("empty") << QByteArray() << QByteArray() << (int) QDataStream::ReadPastEnd << qint64(0);
    QTest::newRow("end 1") << QByteArray("", 1) << QByteArray("", 1) << (int) QDataStream::ReadPastEnd << qint64(0);
    QTest::newRow("end 2") << QByteArray("\x25", 1) << QByteArray("\x25", 1) << (int) QDataStream::ReadPastEnd << qint64(0);
    QTest::newRow("end 3") << QByteArray("11", 2) << QByteArray("11", 2) << (int) QDataStream::ReadPastEnd << qint64(0);
    QTest::newRow("end 4") << QByteArray("111", 3) << QByteArray("111", 3) << (int) QDataStream::ReadPastEnd << qint64(0);
}

TEST_qint(qint32, quint32)

void tst_QDataStream::status_qint64_data()
{
    QTest::addColumn<QByteArray>("bigEndianData");
    QTest::addColumn<QByteArray>("littleEndianData");
    QTest::addColumn<int>("expectedStatus");
    QTest::addColumn<qint64>("expectedValue");

    // ok
    QTest::newRow("0") << QByteArray(8, '\x0') << QByteArray(8, '\x0') << (int) QDataStream::Ok << qint64(0);
    QTest::newRow("-1") << QByteArray("\xff\xff\xff\xff\xff\xff\xff\xff", 8) << QByteArray("\xff\xff\xff\xff\xff\xff\xff\xff", 8) << (int) QDataStream::Ok << qint64(-1);
    QTest::newRow("1") << QByteArray("\x00\x00\x00\x00\x00\x00\x00\x01", 8) << QByteArray("\x01\x00\x00\x00\x00\x00\x00\x00", 8) << (int) QDataStream::Ok << qint64(1);
    QTest::newRow("37") << QByteArray("\x00\x00\x00\x00\x00\x00\x00\x25", 8) << QByteArray("\x25\x00\x00\x00\x00\x00\x00\x00", 8) << (int) QDataStream::Ok << qint64(37);
    QTest::newRow("37j") << QByteArray("\x00\x00\x00\x00\x00\x00\x00\x25j", 9) << QByteArray("\x25\x00\x00\x00\x00\x00\x00\x00j", 9) << (int) QDataStream::Ok << qint64(37);
    QTest::newRow("0x123456789ABCDEF0") << QByteArray("\x12\x34\x56\x78\x9a\xbc\xde\xf0", 8) << QByteArray("\xf0\xde\xbc\x9a\x78\x56\x34\x12", 8) << (int) QDataStream::Ok << (qint64)Q_INT64_C(0x123456789ABCDEF0);

    // past end
    QTest::newRow("empty") << QByteArray() << QByteArray() << (int) QDataStream::ReadPastEnd << qint64(0);
    QTest::newRow("end 1") << QByteArray("", 1) << QByteArray("", 1) << (int) QDataStream::ReadPastEnd << qint64(0);
    QTest::newRow("end 2") << QByteArray("\x25", 1) << QByteArray("\x25", 1) << (int) QDataStream::ReadPastEnd << qint64(0);
    QTest::newRow("end 3") << QByteArray("11", 2) << QByteArray("11", 2) << (int) QDataStream::ReadPastEnd << qint64(0);
    QTest::newRow("end 4") << QByteArray("111", 3) << QByteArray("111", 3) << (int) QDataStream::ReadPastEnd << qint64(0);
    QTest::newRow("end 5") << QByteArray("1111", 4) << QByteArray("1111", 4) << (int) QDataStream::ReadPastEnd << qint64(0);
    QTest::newRow("end 6") << QByteArray("11111", 5) << QByteArray("11111", 5) << (int) QDataStream::ReadPastEnd << qint64(0);
    QTest::newRow("end 7") << QByteArray("111111", 6) << QByteArray("111111", 6) << (int) QDataStream::ReadPastEnd << qint64(0);
    QTest::newRow("end 8") << QByteArray("1111111", 7) << QByteArray("1111111", 7) << (int) QDataStream::ReadPastEnd << qint64(0);
}

TEST_qint(qint64, quint64)

void tst_QDataStream::status_float_data()
{
    QTest::addColumn<QByteArray>("bigEndianData");
    QTest::addColumn<QByteArray>("littleEndianData");
    QTest::addColumn<int>("expectedStatus");
    QTest::addColumn<double>("expectedValue");

    // ok
    QTest::newRow("0") << QByteArray(4, '\0') << QByteArray(4, '\0') << (int) QDataStream::Ok << (double) 0.0;
    QTest::newRow("-1") << QByteArray("\xbf\x80\x00\x00", 4) << QByteArray("\x00\x00\x80\xbf", 4) << (int) QDataStream::Ok << (double) -1;
    QTest::newRow("1") << QByteArray("\x3f\x80\x00\x00", 4) << QByteArray("\x00\x00\x80\x3f", 4) << (int) QDataStream::Ok << (double) 1;
    QTest::newRow("37") << QByteArray("\x42\x14\x00\x00", 4) << QByteArray("\x00\x00\x14\x42", 4) << (int) QDataStream::Ok << (double) 37;
    QTest::newRow("37j") << QByteArray("\x42\x14\x00\x00j", 5) << QByteArray("\x00\x00\x14\x42j", 5) << (int) QDataStream::Ok << (double) 37;
    QTest::newRow("3.14") << QByteArray("\x40\x48\xf5\xc3", 4) << QByteArray("\xc3\xf5\x48\x40", 4) << (int) QDataStream::Ok << (double) 3.14;

    // past end
    QTest::newRow("empty") << QByteArray() << QByteArray() << (int) QDataStream::ReadPastEnd << double(0);
    QTest::newRow("end 1") << QByteArray("", 1) << QByteArray("", 1) << (int) QDataStream::ReadPastEnd << double(0);
    QTest::newRow("end 2") << QByteArray("\x25", 1) << QByteArray("\x25", 1) << (int) QDataStream::ReadPastEnd << double(0);
    QTest::newRow("end 3") << QByteArray("11", 2) << QByteArray("11", 2) << (int) QDataStream::ReadPastEnd << double(0);
    QTest::newRow("end 4") << QByteArray("111", 3) << QByteArray("111", 3) << (int) QDataStream::ReadPastEnd << double(0);
}

TEST_FLOAT(float)

void tst_QDataStream::status_double_data()
{
    QTest::addColumn<QByteArray>("bigEndianData");
    QTest::addColumn<QByteArray>("littleEndianData");
    QTest::addColumn<int>("expectedStatus");
    QTest::addColumn<double>("expectedValue");

    // ok
    QTest::newRow("0") << QByteArray("\x00\x00\x00\x00\x00\x00\x00\x00", 8) << QByteArray("\x00\x00\x00\x00\x00\x00\x00\x00", 8) << (int) QDataStream::Ok << (double) 0;
    QTest::newRow("-1") << QByteArray("\xbf\xf0\x00\x00\x00\x00\x00\x00", 8) << QByteArray("\x00\x00\x00\x00\x00\x00\xf0\xbf", 8) << (int) QDataStream::Ok << (double) -1;
    QTest::newRow("1") << QByteArray("\x3f\xf0\x00\x00\x00\x00\x00\x00", 8) << QByteArray("\x00\x00\x00\x00\x00\x00\xf0\x3f", 8) << (int) QDataStream::Ok << (double) 1;
    QTest::newRow("37") << QByteArray("\x40\x42\x80\x00\x00\x00\x00\x00", 8) << QByteArray("\x00\x00\x00\x00\x00\x80\x42\x40", 8) << (int) QDataStream::Ok << (double) 37;
    QTest::newRow("37j") << QByteArray("\x40\x42\x80\x00\x00\x00\x00\x00j", 9) << QByteArray("\x00\x00\x00\x00\x00\x80\x42\x40j", 9) << (int) QDataStream::Ok << (double) 37;
    QTest::newRow("3.14") << QByteArray("\x40\x09\x1e\xb8\x60\x00\x00\x00", 8) << QByteArray("\x00\x00\x00\x60\xb8\x1e\x09\x40", 8) << (int) QDataStream::Ok << (double) 3.14;
    QTest::newRow("1234.5678") << QByteArray("\x40\x93\x4a\x45\x6d\x5c\xfa\xad", 8) << QByteArray("\xad\xfa\x5c\x6d\x45\x4a\x93\x40", 8) << (int) QDataStream::Ok << (double) 1234.5678;

    // past end
    QTest::newRow("empty") << QByteArray() << QByteArray() << (int) QDataStream::ReadPastEnd << double(0);
    QTest::newRow("end 1") << QByteArray("", 1) << QByteArray("", 1) << (int) QDataStream::ReadPastEnd << double(0);
    QTest::newRow("end 2") << QByteArray("\x25", 1) << QByteArray("\x25", 1) << (int) QDataStream::ReadPastEnd << double(0);
    QTest::newRow("end 3") << QByteArray("11", 2) << QByteArray("11", 2) << (int) QDataStream::ReadPastEnd << double(0);
    QTest::newRow("end 4") << QByteArray("111", 3) << QByteArray("111", 3) << (int) QDataStream::ReadPastEnd << double(0);
    QTest::newRow("end 5") << QByteArray("1111", 4) << QByteArray("1111", 4) << (int) QDataStream::ReadPastEnd << double(0);
    QTest::newRow("end 6") << QByteArray("11111", 5) << QByteArray("11111", 5) << (int) QDataStream::ReadPastEnd << double(0);
    QTest::newRow("end 7") << QByteArray("111111", 6) << QByteArray("111111", 6) << (int) QDataStream::ReadPastEnd << double(0);
    QTest::newRow("end 8") << QByteArray("1111111", 7) << QByteArray("1111111", 7) << (int) QDataStream::ReadPastEnd << double(0);
}

TEST_FLOAT(double)

void tst_QDataStream::status_charptr_QByteArray_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<int>("expectedStatus");
    QTest::addColumn<QByteArray>("expectedString");

#if !defined(Q_OS_WINCE)
    QByteArray oneMbMinus1(1024 * 1024 - 1, '\0');
    for (int i = 0; i < oneMbMinus1.size(); ++i)
        oneMbMinus1[i] = 0x1 | (8 * ((uchar)i / 9));
    QByteArray threeMbMinus1 = oneMbMinus1 + 'j' + oneMbMinus1 + 'k' + oneMbMinus1;
#endif

    // ok
    QTest::newRow("size 0") << QByteArray("\x00\x00\x00\x00", 4) << (int) QDataStream::Ok << QByteArray();
    QTest::newRow("size 1") << QByteArray("\x00\x00\x00\x01j", 5) << (int) QDataStream::Ok << QByteArray("j");
    QTest::newRow("size 2") << QByteArray("\x00\x00\x00\x02jk", 6) << (int) QDataStream::Ok << QByteArray("jk");
    QTest::newRow("size 3") << QByteArray("\x00\x00\x00\x03jkl", 7) << (int) QDataStream::Ok << QByteArray("jkl");
    QTest::newRow("size 4") << QByteArray("\x00\x00\x00\x04jklm", 8) << (int) QDataStream::Ok << QByteArray("jklm");
    QTest::newRow("size 4j") << QByteArray("\x00\x00\x00\x04jklmj", 8) << (int) QDataStream::Ok << QByteArray("jklm");
#if !defined(Q_OS_WINCE)
    QTest::newRow("size 1MB-1") << QByteArray("\x00\x0f\xff\xff", 4) + oneMbMinus1 + QByteArray("j") << (int) QDataStream::Ok << oneMbMinus1;
    QTest::newRow("size 1MB") << QByteArray("\x00\x10\x00\x00", 4) + oneMbMinus1 + QByteArray("jkl") << (int) QDataStream::Ok << oneMbMinus1 + "j";
    QTest::newRow("size 1MB+1") << QByteArray("\x00\x10\x00\x01", 4) + oneMbMinus1 + QByteArray("jkl") << (int) QDataStream::Ok << oneMbMinus1 + "jk";
    QTest::newRow("size 3MB-1") << QByteArray("\x00\x2f\xff\xff", 4) + threeMbMinus1 + QByteArray("j") << (int) QDataStream::Ok << threeMbMinus1;
    QTest::newRow("size 3MB") << QByteArray("\x00\x30\x00\x00", 4) + threeMbMinus1 + QByteArray("jkl") << (int) QDataStream::Ok << threeMbMinus1 + "j";
    QTest::newRow("size 3MB+1") << QByteArray("\x00\x30\x00\x01", 4) + threeMbMinus1 + QByteArray("jkl") << (int) QDataStream::Ok << threeMbMinus1 + "jk";
#endif

    // past end
    QTest::newRow("empty") << QByteArray() << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("trunclen 1") << QByteArray("x") << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("trunclen 2") << QByteArray("xx") << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("trunclen 3") << QByteArray("xxx") << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("truncdata 1") << QByteArray("xxxx") << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("truncdata 2") << QByteArray("xxxxyyyy") << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("badsize 1") << QByteArray("\x00\x00\x00\x01", 4) << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("badsize 2") << QByteArray("\x00\x00\x00\x02j", 5) << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("badsize 3") << QByteArray("\x00\x00\x00\x03jk", 6) << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("badsize 4") << QByteArray("\x00\x00\x00\x04jkl", 7) << (int) QDataStream::ReadPastEnd << QByteArray();
#if !defined(Q_OS_WINCE)
    QTest::newRow("badsize 1MB") << QByteArray("\x00\x10\x00\x00", 4) + oneMbMinus1 << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("badsize 1MB+1") << QByteArray("\x00\x10\x00\x01", 4) + oneMbMinus1 + QByteArray("j") << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("badsize 3MB") << QByteArray("\x00\x30\x00\x00", 4) + threeMbMinus1 << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("badsize 3MB+1") << QByteArray("\x00\x30\x00\x01", 4) + threeMbMinus1 + QByteArray("j") << (int) QDataStream::ReadPastEnd << QByteArray();
#endif
    QTest::newRow("size -1") << QByteArray("\xff\xff\xff\xff", 4) << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("size -2") << QByteArray("\xff\xff\xff\xfe", 4) << (int) QDataStream::ReadPastEnd << QByteArray();
}

void tst_QDataStream::status_charptr_QByteArray()
{
    QFETCH(QByteArray, data);
    QFETCH(int, expectedStatus);
    QFETCH(QByteArray, expectedString);

    {
        QDataStream stream(&data, QIODevice::ReadOnly);
        char *buf;
        stream >> buf;

        QCOMPARE((int)qstrlen(buf), expectedString.size());
        QCOMPARE(QByteArray(buf), expectedString);
        QCOMPARE(int(stream.status()), expectedStatus);
        delete [] buf;
    }
    {
        QDataStream stream(&data, QIODevice::ReadOnly);
        char *buf;
        uint len;
        stream.readBytes(buf, len);

        QCOMPARE((int)len, expectedString.size());
        QCOMPARE(QByteArray(buf, len), expectedString);
        QCOMPARE(int(stream.status()), expectedStatus);
        delete [] buf;
    }
    {
        QDataStream stream(&data, QIODevice::ReadOnly);
        QByteArray buf;
        stream >> buf;

        if (data.startsWith("\xff\xff\xff\xff")) {
            // QByteArray, unlike 'char *', supports the null/empty distinction
            QVERIFY(buf.isNull());
        } else {
            QCOMPARE(buf.size(), expectedString.size());
            QCOMPARE(buf, expectedString);
            QCOMPARE(int(stream.status()), expectedStatus);
        }
    }
}

static QByteArray qstring2qbytearray(const QString &str)
{
    QByteArray ba(str.size() * 2 , '\0');
    for (int i = 0; i < str.size(); ++i) {
        // BigEndian
        ba[2 * i] = str[i].row();
        ba[2 * i + 1] = str[i].cell();
    }
    return ba;
}

void tst_QDataStream::status_QString_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<int>("expectedStatus");
    QTest::addColumn<QString>("expectedString");

#if !defined(Q_OS_WINCE)
    QString oneMbMinus1;
    oneMbMinus1.resize(1024 * 1024 - 1);
    for (int i = 0; i < oneMbMinus1.size(); ++i)
        oneMbMinus1[i] = 0x1 | (8 * ((uchar)i / 9));
    QString threeMbMinus1 = oneMbMinus1 + QChar('j') + oneMbMinus1 + QChar('k') + oneMbMinus1;

    QByteArray threeMbMinus1Data = qstring2qbytearray(threeMbMinus1);
    QByteArray oneMbMinus1Data = qstring2qbytearray(oneMbMinus1);
#endif

    // ok
    QTest::newRow("size 0") << QByteArray("\x00\x00\x00\x00", 4) << (int) QDataStream::Ok << QString();
    QTest::newRow("size 1") << QByteArray("\x00\x00\x00\x02\x00j", 6) << (int) QDataStream::Ok << QString("j");
    QTest::newRow("size 2") << QByteArray("\x00\x00\x00\x04\x00j\x00k", 8) << (int) QDataStream::Ok << QString("jk");
    QTest::newRow("size 3") << QByteArray("\x00\x00\x00\x06\x00j\x00k\x00l", 10) << (int) QDataStream::Ok << QString("jkl");
    QTest::newRow("size 4") << QByteArray("\x00\x00\x00\x08\x00j\x00k\x00l\x00m", 12) << (int) QDataStream::Ok << QString("jklm");
    QTest::newRow("size 4j") << QByteArray("\x00\x00\x00\x08\x00j\x00k\x00l\x00mjj", 14) << (int) QDataStream::Ok << QString("jklm");
#if !defined(Q_OS_WINCE)
    QTest::newRow("size 1MB-1") << QByteArray("\x00\x1f\xff\xfe", 4) + oneMbMinus1Data + QByteArray("jj") << (int) QDataStream::Ok << oneMbMinus1;
    QTest::newRow("size 1MB") << QByteArray("\x00\x20\x00\x00", 4) + oneMbMinus1Data + QByteArray("\x00j\x00k\x00l", 6) << (int) QDataStream::Ok << oneMbMinus1 + "j";
    QTest::newRow("size 1MB+1") << QByteArray("\x00\x20\x00\x02", 4) + oneMbMinus1Data + QByteArray("\x00j\x00k\x00l", 6) << (int) QDataStream::Ok << oneMbMinus1 + "jk";
    QTest::newRow("size 3MB-1") << QByteArray("\x00\x5f\xff\xfe", 4) + threeMbMinus1Data + QByteArray("jj") << (int) QDataStream::Ok << threeMbMinus1;
    QTest::newRow("size 3MB") << QByteArray("\x00\x60\x00\x00", 4) + threeMbMinus1Data + QByteArray("\x00j\x00k\x00l", 6) << (int) QDataStream::Ok << threeMbMinus1 + "j";
    QTest::newRow("size 3MB+1") << QByteArray("\x00\x60\x00\x02", 4) + threeMbMinus1Data + QByteArray("\x00j\x00k\x00l", 6) << (int) QDataStream::Ok << threeMbMinus1 + "jk";
#endif

    // past end
    QTest::newRow("empty") << QByteArray() << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("trunclen 1") << QByteArray("x") << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("trunclen 2") << QByteArray("xx") << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("trunclen 3") << QByteArray("xxx") << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("truncdata 1") << QByteArray("xxxx") << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("truncdata 2") << QByteArray("xxxxyyyy") << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("badsize 1") << QByteArray("\x00\x00\x00\x02", 4) << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("badsize 2") << QByteArray("\x00\x00\x00\x04jj", 6) << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("badsize 3") << QByteArray("\x00\x00\x00\x06jjkk", 8) << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("badsize 4") << QByteArray("\x00\x00\x00\x08jjkkll", 10) << (int) QDataStream::ReadPastEnd << QString();
#if !defined(Q_OS_WINCE)
    QTest::newRow("badsize 1MB") << QByteArray("\x00\x20\x00\x00", 4) + oneMbMinus1Data << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("badsize 1MB+1") << QByteArray("\x00\x20\x00\x02", 4) + oneMbMinus1Data + QByteArray("j") << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("badsize 3MB") << QByteArray("\x00\x60\x00\x00", 4) + threeMbMinus1Data << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("badsize 3MB+1") << QByteArray("\x00\x60\x00\x02", 4) + threeMbMinus1Data + QByteArray("j") << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("size -2") << QByteArray("\xff\xff\xff\xfe", 4) << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("size MAX") << QByteArray("\x7f\xff\xff\xfe", 4) << (int) QDataStream::ReadPastEnd << QString();
#endif

    // corrupt data
    QTest::newRow("corrupt1") << QByteArray("yyyy") << (int) QDataStream::ReadCorruptData << QString();
    QTest::newRow("size -3") << QByteArray("\xff\xff\xff\xfd", 4) << (int) QDataStream::ReadCorruptData << QString();
}

void tst_QDataStream::status_QString()
{
    QFETCH(QByteArray, data);
    QFETCH(int, expectedStatus);
    QFETCH(QString, expectedString);

    QDataStream stream(&data, QIODevice::ReadOnly);
    QString str;
    stream >> str;

    QCOMPARE(str.size(), expectedString.size());
    QCOMPARE(str, expectedString);
    QCOMPARE(int(stream.status()), expectedStatus);
}

static QBitArray bitarray(const QString &str)
{
    QBitArray array(str.size());
    for (int i = 0; i < str.size(); ++i)
        array[i] = (str[i] != '0');
    return array;
}

void tst_QDataStream::status_QBitArray_data()
{
    QTest::addColumn<QByteArray>("data");
    QTest::addColumn<int>("expectedStatus");
    QTest::addColumn<QBitArray>("expectedString");

    // ok
    QTest::newRow("size 0") << QByteArray("\x00\x00\x00\x00", 4) << (int) QDataStream::Ok << QBitArray();
    QTest::newRow("size 1a") << QByteArray("\x00\x00\x00\x01\x00", 5) << (int) QDataStream::Ok << bitarray("0");
    QTest::newRow("size 1b") << QByteArray("\x00\x00\x00\x01\x01", 5) << (int) QDataStream::Ok << bitarray("1");
    QTest::newRow("size 2") << QByteArray("\x00\x00\x00\x02\x03", 5) << (int) QDataStream::Ok << bitarray("11");
    QTest::newRow("size 3") << QByteArray("\x00\x00\x00\x03\x07", 5) << (int) QDataStream::Ok << bitarray("111");
    QTest::newRow("size 4") << QByteArray("\x00\x00\x00\x04\x0f", 5) << (int) QDataStream::Ok << bitarray("1111");
    QTest::newRow("size 5") << QByteArray("\x00\x00\x00\x05\x1f", 5) << (int) QDataStream::Ok << bitarray("11111");
    QTest::newRow("size 6") << QByteArray("\x00\x00\x00\x06\x3f", 5) << (int) QDataStream::Ok << bitarray("111111");
    QTest::newRow("size 7a") << QByteArray("\x00\x00\x00\x07\x7f", 5) << (int) QDataStream::Ok << bitarray("1111111");
    QTest::newRow("size 7b") << QByteArray("\x00\x00\x00\x07\x7e", 5) << (int) QDataStream::Ok << bitarray("0111111");
    QTest::newRow("size 7c") << QByteArray("\x00\x00\x00\x07\x00", 5) << (int) QDataStream::Ok << bitarray("0000000");
    QTest::newRow("size 7d") << QByteArray("\x00\x00\x00\x07\x39", 5) << (int) QDataStream::Ok << bitarray("1001110");
    QTest::newRow("size 8") << QByteArray("\x00\x00\x00\x08\xff", 5) << (int) QDataStream::Ok << bitarray("11111111");
    QTest::newRow("size 9") << QByteArray("\x00\x00\x00\x09\xff\x01", 6) << (int) QDataStream::Ok << bitarray("111111111");
    QTest::newRow("size 15") << QByteArray("\x00\x00\x00\x0f\xff\x7f", 6) << (int) QDataStream::Ok << bitarray("111111111111111");
    QTest::newRow("size 16") << QByteArray("\x00\x00\x00\x10\xff\xff", 6) << (int) QDataStream::Ok << bitarray("1111111111111111");
    QTest::newRow("size 17") << QByteArray("\x00\x00\x00\x11\xff\xff\x01", 7) << (int) QDataStream::Ok << bitarray("11111111111111111");
    QTest::newRow("size 32") << QByteArray("\x00\x00\x00\x20\xff\xff\xff\xff", 8) << (int) QDataStream::Ok << bitarray("11111111111111111111111111111111");

    // past end
    QTest::newRow("empty") << QByteArray() << (int) QDataStream::ReadPastEnd << QBitArray();
    QTest::newRow("badsize 0a") << QByteArray("\x00", 1) << (int) QDataStream::ReadPastEnd << QBitArray();
    QTest::newRow("badsize 0b") << QByteArray("\x00\x00", 2) << (int) QDataStream::ReadPastEnd << QBitArray();
    QTest::newRow("badsize 0c") << QByteArray("\x00\x00\x00", 3) << (int) QDataStream::ReadPastEnd << QBitArray();
    QTest::newRow("badsize 1") << QByteArray("\x00\x00\x00\x01", 4) << (int) QDataStream::ReadPastEnd << QBitArray();
    QTest::newRow("badsize 2") << QByteArray("\x00\x00\x00\x02", 4) << (int) QDataStream::ReadPastEnd << QBitArray();
    QTest::newRow("badsize 3") << QByteArray("\x00\x00\x00\x03", 4) << (int) QDataStream::ReadPastEnd << QBitArray();
    QTest::newRow("badsize 7") << QByteArray("\x00\x00\x00\x04", 4) << (int) QDataStream::ReadPastEnd << QBitArray();
    QTest::newRow("size 8") << QByteArray("\x00\x00\x00\x08", 4) << (int) QDataStream::ReadPastEnd << QBitArray();
    QTest::newRow("size 9") << QByteArray("\x00\x00\x00\x09\xff", 5) << (int) QDataStream::ReadPastEnd << QBitArray();
    QTest::newRow("size 15") << QByteArray("\x00\x00\x00\x0f\xff", 5) << (int) QDataStream::ReadPastEnd << QBitArray();
    QTest::newRow("size 16") << QByteArray("\x00\x00\x00\x10\xff", 5) << (int) QDataStream::ReadPastEnd << QBitArray();
    QTest::newRow("size 17") << QByteArray("\x00\x00\x00\x11\xff\xff", 6) << (int) QDataStream::ReadPastEnd << QBitArray();
    QTest::newRow("size 32") << QByteArray("\x00\x00\x00\x20\xff\xff\xff", 7) << (int) QDataStream::ReadPastEnd << QBitArray();

    // corrupt data
    QTest::newRow("junk 1a") << QByteArray("\x00\x00\x00\x01\x02", 5) << (int) QDataStream::ReadCorruptData << QBitArray();
    QTest::newRow("junk 1b") << QByteArray("\x00\x00\x00\x01\x04", 5) << (int) QDataStream::ReadCorruptData << QBitArray();
    QTest::newRow("junk 1c") << QByteArray("\x00\x00\x00\x01\x08", 5) << (int) QDataStream::ReadCorruptData << QBitArray();
    QTest::newRow("junk 1d") << QByteArray("\x00\x00\x00\x01\x10", 5) << (int) QDataStream::ReadCorruptData << QBitArray();
    QTest::newRow("junk 1e") << QByteArray("\x00\x00\x00\x01\x20", 5) << (int) QDataStream::ReadCorruptData << QBitArray();
    QTest::newRow("junk 1f") << QByteArray("\x00\x00\x00\x01\x40", 5) << (int) QDataStream::ReadCorruptData << QBitArray();
    QTest::newRow("junk 1g") << QByteArray("\x00\x00\x00\x01\x80", 5) << (int) QDataStream::ReadCorruptData << QBitArray();
    QTest::newRow("junk 2") << QByteArray("\x00\x00\x00\x02\x04", 5) << (int) QDataStream::ReadCorruptData << QBitArray();
    QTest::newRow("junk 3") << QByteArray("\x00\x00\x00\x03\x08", 5) << (int) QDataStream::ReadCorruptData << QBitArray();
    QTest::newRow("junk 4") << QByteArray("\x00\x00\x00\x04\x10", 5) << (int) QDataStream::ReadCorruptData << QBitArray();
    QTest::newRow("junk 5") << QByteArray("\x00\x00\x00\x05\x20", 5) << (int) QDataStream::ReadCorruptData << QBitArray();
    QTest::newRow("junk 6") << QByteArray("\x00\x00\x00\x06\x40", 5) << (int) QDataStream::ReadCorruptData << QBitArray();
    QTest::newRow("junk 7") << QByteArray("\x00\x00\x00\x07\x80", 5) << (int) QDataStream::ReadCorruptData << QBitArray();
}

void tst_QDataStream::status_QBitArray()
{
    QFETCH(QByteArray, data);
    QFETCH(int, expectedStatus);
    QFETCH(QBitArray, expectedString);

    QDataStream stream(&data, QIODevice::ReadOnly);
    QBitArray str;
    stream >> str;

    QCOMPARE(int(stream.status()), expectedStatus);
    QCOMPARE(str.size(), expectedString.size());
    QCOMPARE(str, expectedString);
}

#define MAP_TEST(byteArray, expectedStatus, expectedHash) \
    { \
        QByteArray ba = byteArray; \
        QDataStream stream(&ba, QIODevice::ReadOnly); \
        stream >> hash; \
        QCOMPARE((int)stream.status(), (int)expectedStatus); \
        QCOMPARE(hash.size(), expectedHash.size()); \
        QCOMPARE(hash, expectedHash); \
    } \
    { \
        QByteArray ba = byteArray; \
        StringMap expectedMap; \
        StringHash::const_iterator it = expectedHash.constBegin(); \
        for (; it != expectedHash.constEnd(); ++it) \
            expectedMap.insert(it.key(), it.value()); \
        QDataStream stream(&ba, QIODevice::ReadOnly); \
        stream >> map; \
        QCOMPARE((int)stream.status(), (int)expectedStatus); \
        QCOMPARE(map.size(), expectedMap.size()); \
        QCOMPARE(map, expectedMap); \
    }

void tst_QDataStream::status_QHash_QMap()
{
    typedef QHash<QString, QString> StringHash;
    typedef QMap<QString, QString> StringMap;
    StringHash hash;
    StringMap map;

    StringHash hash1;
    hash1.insert("", "");

    StringHash hash2;
    hash2.insert("J", "K");
    hash2.insert("L", "MN");

    // ok
    MAP_TEST(QByteArray("\x00\x00\x00\x00", 4), QDataStream::Ok, StringHash());
    MAP_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00", 12), QDataStream::Ok, hash1);
    MAP_TEST(QByteArray("\x00\x00\x00\x02\x00\x00\x00\x02\x00J\x00\x00\x00\x02\x00K"
                        "\x00\x00\x00\x02\x00L\x00\x00\x00\x04\x00M\x00N", 30), QDataStream::Ok, hash2);

    // past end
    MAP_TEST(QByteArray(), QDataStream::ReadPastEnd, StringHash());
    MAP_TEST(QByteArray("\x00", 1), QDataStream::ReadPastEnd, StringHash());
    MAP_TEST(QByteArray("\x00\x00", 2), QDataStream::ReadPastEnd, StringHash());
    MAP_TEST(QByteArray("\x00\x00\x00", 3), QDataStream::ReadPastEnd, StringHash());
    MAP_TEST(QByteArray("\x00\x00\x00\x01", 4), QDataStream::ReadPastEnd, StringHash());
    for (int i = 4; i < 12; ++i) {
        MAP_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00", i), QDataStream::ReadPastEnd, StringHash());
    }

    // corrupt data
    MAP_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x01", 8), QDataStream::ReadCorruptData, StringHash());
    MAP_TEST(QByteArray("\x00\x00\x00\x02\x00\x00\x00\x01\x00J\x00\x00\x00\x01\x00K"
                        "\x00\x00\x00\x01\x00L\x00\x00\x00\x02\x00M\x00N", 30), QDataStream::ReadCorruptData, StringHash());
}

#define LIST_TEST(byteArray, expectedStatus, expectedList) \
    { \
        QByteArray ba = byteArray; \
        QDataStream stream(&ba, QIODevice::ReadOnly); \
        stream >> list; \
        QCOMPARE((int)stream.status(), (int)expectedStatus); \
        QCOMPARE(list.size(), expectedList.size()); \
        QCOMPARE(list, expectedList); \
    } \
    { \
        LinkedList expectedLinkedList; \
        for (int i = 0; i < expectedList.count(); ++i) \
            expectedLinkedList << expectedList.at(i); \
        QByteArray ba = byteArray; \
        QDataStream stream(&ba, QIODevice::ReadOnly); \
        stream >> linkedList; \
        QCOMPARE((int)stream.status(), (int)expectedStatus); \
        QCOMPARE(linkedList.size(), expectedLinkedList.size()); \
        QCOMPARE(linkedList, expectedLinkedList); \
    } \
    { \
        Vector expectedVector; \
        for (int i = 0; i < expectedList.count(); ++i) \
            expectedVector << expectedList.at(i); \
        QByteArray ba = byteArray; \
        QDataStream stream(&ba, QIODevice::ReadOnly); \
        stream >> vector; \
        QCOMPARE((int)stream.status(), (int)expectedStatus); \
        QCOMPARE(vector.size(), expectedVector.size()); \
        QCOMPARE(vector, expectedVector); \
    }

void tst_QDataStream::status_QLinkedList_QList_QVector()
{
    typedef QLinkedList<QString> LinkedList;
    typedef QList<QString> List;
    typedef QVector<QString> Vector;
    LinkedList linkedList;
    List list;
    Vector vector;

    LIST_TEST(QByteArray(), QDataStream::ReadPastEnd, List());
    LIST_TEST(QByteArray("\x00\x00\x00\x00", 4), QDataStream::Ok, List());
}

void tst_QDataStream::streamToAndFromQByteArray()
{
    QByteArray data;
    QDataStream in(&data, QIODevice::WriteOnly);
    QDataStream out(&data, QIODevice::ReadOnly);

    quint32 x = 0xdeadbeef;
    quint32 y;
    in << x;
    out >> y;

    QCOMPARE(y, x);
}

void tst_QDataStream::streamRealDataTypes()
{
    // Generate QPicture from pixmap.
    QPixmap pm(open_xpm);
    QVERIFY(!pm.isNull());
    QPicture picture;
    picture.setBoundingRect(QRect(QPoint(0, 0), pm.size()));
    QPainter painter(&picture);
    painter.drawPixmap(0, 0, pm);
    painter.end();

    // Generate path
    QPainterPath path;
    path.lineTo(10, 0);
    path.cubicTo(0, 0, 10, 10, 20, 20);
    path.arcTo(4, 5, 6, 7, 8, 9);
    path.quadTo(1, 2, 3, 4);

    QColor color(64, 64, 64);
    color.setAlphaF(0.5);
    QRadialGradient radialGradient(5, 6, 7, 8, 9);
    QBrush radialBrush(radialGradient);
    QConicalGradient conicalGradient(5, 6, 7);
    QBrush conicalBrush(conicalGradient);

    for (int i = 0; i < 2; ++i) {
        QFile file;
        if (i == 0) {
            file.setFileName(QFINDTESTDATA("datastream.q42"));
        } else {
            file.setFileName("datastream.tmp");

            // Generate data
            QVERIFY(file.open(QIODevice::WriteOnly));
            QDataStream stream(&file);
            stream.setVersion(QDataStream::Qt_4_2);
            stream << qreal(0) << qreal(1.0) << qreal(1.1) << qreal(3.14) << qreal(-3.14) << qreal(-1);
            stream << QPointF(3, 5) << QRectF(-1, -2, 3, 4) << (QPolygonF() << QPointF(0, 0) << QPointF(1, 2));
            stream << QMatrix().rotate(90).scale(2, 2);
            stream << path;
            stream << picture;
            stream << QTextLength(QTextLength::VariableLength, 1.5);
            stream << color;
            stream << radialBrush << conicalBrush;
            stream << QPen(QBrush(Qt::red), 1.5);

            file.close();
        }

        QPointF point;
        QRectF rect;
        QPolygonF polygon;
        QMatrix matrix;
        QPainterPath p;
        QPicture pict;
        QTextLength textLength;
        QColor col;
        QBrush rGrad;
        QBrush cGrad;
        QPen pen;

        QVERIFY(file.open(QIODevice::ReadOnly));
        QDataStream stream(&file);
        stream.setVersion(QDataStream::Qt_4_2);

        if (i == 0) {
            // the reference stream for 4.2 contains doubles,
            // so we must read them out as doubles!
            double a, b, c, d, e, f;
            stream >> a;
            QCOMPARE(a, 0.0);
            stream >> b;
            QCOMPARE(b, 1.0);
            stream >> c;
            QCOMPARE(c, 1.1);
            stream >> d;
            QCOMPARE(d, 3.14);
            stream >> e;
            QCOMPARE(e, -3.14);
            stream >> f;
            QCOMPARE(f, -1.0);
        } else {
            qreal a, b, c, d, e, f;
            stream >> a;
            QCOMPARE(a, qreal(0));
            stream >> b;
            QCOMPARE(b, qreal(1.0));
            stream >> c;
            QCOMPARE(c, qreal(1.1));
            stream >> d;
            QCOMPARE(d, qreal(3.14));
            stream >> e;
            QCOMPARE(e, qreal(-3.14));
            stream >> f;
            QCOMPARE(f, qreal(-1));
        }
        stream >> point;
        QCOMPARE(point, QPointF(3, 5));
        stream >> rect;
        QCOMPARE(rect, QRectF(-1, -2, 3, 4));
        stream >> polygon;
        QCOMPARE((QVector<QPointF> &)polygon, (QPolygonF() << QPointF(0, 0) << QPointF(1, 2)));
        stream >> matrix;
        QCOMPARE(matrix, QMatrix().rotate(90).scale(2, 2));
        stream >> p;
        QCOMPARE(p, path);
        if (i == 1) {
            stream >> pict;

            QByteArray pictA, pictB;
            QBuffer bufA, bufB;
            QVERIFY(bufA.open(QIODevice::ReadWrite));
            QVERIFY(bufB.open(QIODevice::ReadWrite));

            picture.save(&bufA);
            pict.save(&bufB);

            QCOMPARE(pictA, pictB);
        }
        stream >> textLength;
        QCOMPARE(textLength, QTextLength(QTextLength::VariableLength, 1.5));
        stream >> col;
        QCOMPARE(col, color);
        stream >> rGrad;
        QCOMPARE(rGrad.style(), radialBrush.style());
        QCOMPARE(rGrad.matrix(), radialBrush.matrix());
        QCOMPARE(rGrad.gradient()->type(), radialBrush.gradient()->type());
        QCOMPARE(rGrad.gradient()->stops(), radialBrush.gradient()->stops());
        QCOMPARE(rGrad.gradient()->spread(), radialBrush.gradient()->spread());
        QCOMPARE(((QRadialGradient *)rGrad.gradient())->center(), ((QRadialGradient *)radialBrush.gradient())->center());
        QCOMPARE(((QRadialGradient *)rGrad.gradient())->focalPoint(), ((QRadialGradient *)radialBrush.gradient())->focalPoint());
        QCOMPARE(((QRadialGradient *)rGrad.gradient())->radius(), ((QRadialGradient *)radialBrush.gradient())->radius());
        stream >> cGrad;
        QCOMPARE(cGrad.style(), conicalBrush.style());
        QCOMPARE(cGrad.matrix(), conicalBrush.matrix());
        QCOMPARE(cGrad.gradient()->type(), conicalBrush.gradient()->type());
        QCOMPARE(cGrad.gradient()->stops(), conicalBrush.gradient()->stops());
        QCOMPARE(cGrad.gradient()->spread(), conicalBrush.gradient()->spread());
        QCOMPARE(((QConicalGradient *)cGrad.gradient())->center(), ((QConicalGradient *)conicalBrush.gradient())->center());
        QCOMPARE(((QConicalGradient *)cGrad.gradient())->angle(), ((QConicalGradient *)conicalBrush.gradient())->angle());

        QCOMPARE(cGrad, conicalBrush);
        stream >> pen;
        QCOMPARE(pen.widthF(), qreal(1.5));

        QCOMPARE(stream.status(), QDataStream::Ok);
    }
}

void tst_QDataStream::compatibility_Qt3()
{
    QByteArray ba("hello");
    QVariant var = ba;
    const quint32 invalidColor = 0x49000000;
    QByteArray stream;
    {
        QDataStream out(&stream, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_3_3);
        out << var;
        out << QColor();
        out << QColor(Qt::darkYellow);
        out << QColor(Qt::darkCyan);
        out << invalidColor;
    }
    {
        QDataStream in(stream);
        in.setVersion(QDataStream::Qt_3_3);

        quint32 type;
        in >> type;
        //29 is the type of a QByteArray in Qt3
        QCOMPARE(type, quint32(29));
        QByteArray ba2;
        in >> ba2;
        QCOMPARE(ba2, ba);

        quint32 color;
        in >> color;
        QCOMPARE(color, invalidColor);
        in >> color;
        QCOMPARE(color, QColor(Qt::darkYellow).rgb());
        QColor col;
        in >> col;
        QCOMPARE(col, QColor(Qt::darkCyan));
        in >> col;
        QVERIFY(!col.isValid());
    }
    {
        QLinearGradient gradient(QPointF(0,0), QPointF(1,1));
        gradient.setColorAt(0, Qt::red);
        gradient.setColorAt(1, Qt::blue);

        QBrush brush(gradient);
        QPalette palette;
        palette.setBrush(QPalette::Button, brush);
        palette.setColor(QPalette::Light, Qt::green);

        QByteArray stream;
        {
            QDataStream out(&stream, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_3_3);
            out << palette;
            out << brush;
        }
        QBrush in_brush;
        QPalette in_palette;
        {
            QDataStream in(stream);
            in.setVersion(QDataStream::Qt_3_3);
            in >> in_palette;
            in >> in_brush;
        }
        QVERIFY(in_brush.style() == Qt::NoBrush);
        QVERIFY(in_palette.brush(QPalette::Button).style() == Qt::NoBrush);
        QVERIFY(in_palette.color(QPalette::Light) == Qt::green);
    }
}

void tst_QDataStream::compatibility_Qt2()
{
    QLinearGradient gradient(QPointF(0,0), QPointF(1,1));
    gradient.setColorAt(0, Qt::red);
    gradient.setColorAt(1, Qt::blue);

    QBrush brush(gradient);
    QPalette palette;
    palette.setBrush(QPalette::Button, brush);
    palette.setColor(QPalette::Light, Qt::green);

    QByteArray stream;
    {
        QDataStream out(&stream, QIODevice::WriteOnly);
        out.setVersion(QDataStream::Qt_2_1);
        out << palette;
        out << brush;
    }
    QBrush in_brush;
    QPalette in_palette;
    {
        QDataStream in(stream);
        in.setVersion(QDataStream::Qt_2_1);
        in >> in_palette;
        in >> in_brush;
    }
    QVERIFY(in_brush.style() == Qt::NoBrush);
    QVERIFY(in_palette.brush(QPalette::Button).style() == Qt::NoBrush);
    QVERIFY(in_palette.color(QPalette::Light) == Qt::green);
}

void tst_QDataStream::floatingPointNaN()
{
    QDataStream::ByteOrder bo = QSysInfo::ByteOrder == QSysInfo::BigEndian
                                ? QDataStream::LittleEndian
                                : QDataStream::BigEndian;

    // Test and verify that values that become (s)nan's after swapping endianness
    // don't change in the process.
    // When compiling with e.g., MSVC (32bit) and when the fpu is used (fp:precise)
    // all snan's will be converted to qnan's (default behavior).
    // IF we get a snan after swapping endianness we can not copy the value to another
    // float as this will cause the value to differ from the original value.
    QByteArray ba;

    union {
       float f;
       quint32 i;
    } xs[2];

    xs[0].i = qbswap<quint32>(0xff800001);
    xs[1].i = qbswap<quint32>(0x7f800001);

    {
        QDataStream stream(&ba, QIODevice::WriteOnly);
        stream.setByteOrder(bo);
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream << xs[0].f;
        stream << xs[1].f;
    }

    {
        QDataStream stream(ba);
        stream.setByteOrder(bo);
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        float fr = 0.0f;
        stream >> fr;
        QCOMPARE(fr, xs[0].f);
        stream >> fr;
        QCOMPARE(fr, xs[1].f);
    }
}

void tst_QDataStream::floatingPointPrecision()
{
    QByteArray ba;
    {
        QDataStream stream(&ba, QIODevice::WriteOnly);
        QCOMPARE(QDataStream::DoublePrecision, stream.floatingPointPrecision());

        float f = 123.0f;
        stream << f;
        QCOMPARE(ba.size(), int(sizeof(double)));

        double d = 234.0;
        stream << d;
        QCOMPARE(ba.size(), int(sizeof(double)*2));

        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

        f = 123.0f;
        stream << f;
        QCOMPARE(ba.size(), int(sizeof(double)*2 + sizeof(float)));

        d = 234.0;
        stream << d;
        QCOMPARE(ba.size(), int(sizeof(double)*2 + sizeof(float)*2));
    }

    {
        QDataStream stream(ba);

        float f = 0.0f;
        stream >> f;
        QCOMPARE(123.0f, f);

        double d = 0.0;
        stream >> d;
        QCOMPARE(234.0, d);

        f = 0.0f;
        stream.setFloatingPointPrecision(QDataStream::SinglePrecision);
        stream >> f;
        QCOMPARE(123.0f, f);

        d = 0.0;
        stream >> d;
        QCOMPARE(234.0, d);
    }

}

QTEST_MAIN(tst_QDataStream)
#include "tst_qdatastream.moc"

