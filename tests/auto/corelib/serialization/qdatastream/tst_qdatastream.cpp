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
#include <QtGui/QBitmap>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtGui/QPalette>
#include <QtGui/QPen>
#include <QtGui/QPicture>
#include <QtGui/QPixmap>
#include <QtGui/QTextLength>

class tst_QDataStream : public QObject
{
Q_OBJECT

public:
    void stream_data(int noOfElements);

public slots:
    void initTestCase();
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

#ifndef QT_NO_CURSOR
    void stream_QCursor_data();
    void stream_QCursor();
#endif

    void stream_QDate_data();
    void stream_QDate();

    void stream_QTime_data();
    void stream_QTime();

    void stream_QDateTime_data();
    void stream_QDateTime();

    void stream_nullptr_t_data();
    void stream_nullptr_t();

    void stream_QFont_data();
    void stream_QFont();

    void stream_QImage_data();
    void stream_QImage();

    void stream_QPen_data();
    void stream_QPen();

    void stream_QPixmap_data();
    void stream_QPixmap();

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

#if QT_CONFIG(regularexpression)
    void stream_QRegularExpression_data();
    void stream_QRegularExpression();
#endif

    void stream_Map_data();
    void stream_Map();

    void stream_Hash_data();
    void stream_Hash();

    void stream_qint64_data();
    void stream_qint64();

    void stream_QIcon_data();
    void stream_QIcon();

    void stream_QEasingCurve_data();
    void stream_QEasingCurve();

    void stream_atEnd_data();
    void stream_atEnd();

    void stream_writeError();

    void stream_QByteArray2();

    void stream_QJsonDocument();
    void stream_QJsonArray();
    void stream_QJsonObject();
    void stream_QJsonValue();

    void stream_QCborArray();
    void stream_QCborMap();
    void stream_QCborValue();

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

    void enumTest();

    void floatingPointPrecision();

    void compatibility_Qt5();
    void compatibility_Qt3();
    void compatibility_Qt2();

    void floatingPointNaN();

    void transaction_data();
    void transaction();
    void nestedTransactionsResult_data();
    void nestedTransactionsResult();

private:
    void writebool(QDataStream *s);
    void writeQBitArray(QDataStream *s);
    void writeQBrush(QDataStream *s);
    void writeQColor(QDataStream *s);
    void writeQByteArray(QDataStream *s);
    void writenullptr_t(QDataStream *s);
#ifndef QT_NO_CURSOR
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
#if QT_CONFIG(regularexpression)
    void writeQRegularExpression(QDataStream *dev);
#endif
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
    void readnullptr_t(QDataStream *s);
#ifndef QT_NO_CURSOR
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
#if QT_CONFIG(regularexpression)
    void readQRegularExpression(QDataStream *s);
#endif
    void readMap(QDataStream *s);
    void readHash(QDataStream *s);
    void readqint64(QDataStream *s);
    void readQIcon(QDataStream *s);
    void readQEasingCurve(QDataStream *s);

private:
    QSharedPointer<QTemporaryDir> m_tempDir;
    QString m_previousCurrent;
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
    QPalette::ToolTipText + 1,     // Qt_4_6, Qt_4_7, Qt_4_8, Qt_4_9
    QPalette::ToolTipText + 1,     // Qt_5_0
    QPalette::ToolTipText + 1,     // Qt_5_1
    QPalette::ToolTipText + 1,     // Qt_5_2, Qt_5_3
    QPalette::ToolTipText + 1,     // Qt_5_4, Qt_5_5
    QPalette::ToolTipText + 1,     // Qt_5_6, Qt_5_7, Qt_5_8, Qt_5_9, Qt_5_10, Qt_5_11
    QPalette::PlaceholderText + 1, // Qt_5_12
    QPalette::PlaceholderText + 1, // Qt_5_13
    0                              // add the correct value for Qt_5_14 here later
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

void tst_QDataStream::initTestCase()
{
    m_previousCurrent = QDir::currentPath();
    m_tempDir = QSharedPointer<QTemporaryDir>::create();
    QVERIFY2(!m_tempDir.isNull(), qPrintable("Could not create temporary directory."));
    QVERIFY2(QDir::setCurrent(m_tempDir->path()), qPrintable("Could not switch current directory"));
}

void tst_QDataStream::cleanupTestCase()
{
    QFile::remove(QLatin1String("qdatastream.out"));
    QFile::remove(QLatin1String("datastream.tmp"));

    QDir::setCurrent(m_previousCurrent);
}

static int dataIndex(const QString &tag)
{
    int pos = tag.lastIndexOf(QLatin1Char('_'));
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

            QString tag = device + QLatin1Char('_') + byte_order;
            for (int e=0; e<noOfElements; e++) {
                QTest::newRow(qPrintable(tag + QLatin1Char('_') + QString::number(e))) << device << byte_order;
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
    QCOMPARE(V.type(), QVariant::RegExp);
    QCOMPARE(V.toRegExp(), test);
}

// ************************************

#if QT_CONFIG(regularexpression)
static QRegularExpression QRegularExpressionData(int index)
{
    switch (index) {
    case 0: return QRegularExpression();
    case 1: return QRegularExpression("");
    case 2: return QRegularExpression("A", QRegularExpression::CaseInsensitiveOption);
    case 3: return QRegularExpression(QRegularExpression::wildcardToRegularExpression("ABCDE FGHI"));
    case 4: return QRegularExpression(QRegularExpression::anchoredPattern("This is a long string"), QRegularExpression::CaseInsensitiveOption);
    case 5: return QRegularExpression("And again a string with a \nCRLF", QRegularExpression::CaseInsensitiveOption);
    case 6: return QRegularExpression("abcdefghijklmnopqrstuvwxyz ABCDEFGHIJKLMNOPQRESTUVWXYZ 1234567890 ~`!@#$%^&*()_-+={[}]|\\:;\"'<,>.?/", QRegularExpression::InvertedGreedinessOption);
    }
    return QRegularExpression("foo");
}
#define MAX_QREGULAREXPRESSION_DATA 7

void tst_QDataStream::stream_QRegularExpression_data()
{
    stream_data(MAX_QREGULAREXPRESSION_DATA);
}

void tst_QDataStream::stream_QRegularExpression()
{
    STREAM_IMPL(QRegularExpression);
}

void tst_QDataStream::writeQRegularExpression(QDataStream* s)
{
    QRegularExpression test(QRegularExpressionData(dataIndex(QTest::currentDataTag())));
    *s << test;
    *s << QString("Her er det noe tekst");
    *s << test;
    *s << QString("nonempty");
    *s << test;
    *s << QVariant(test);
}

void tst_QDataStream::readQRegularExpression(QDataStream *s)
{
    QRegularExpression R;
    QString S;
    QVariant V;
    QRegularExpression test(QRegularExpressionData(dataIndex(QTest::currentDataTag())));

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
    QCOMPARE(V.type(), QVariant::RegularExpression);
    QCOMPARE(V.toRegularExpression(), test);
}
#endif //QT_CONFIG(regularexpression)

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
    QCOMPARE(d1, expected);
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
    QCOMPARE(d1, expected);
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
    QCOMPARE(d2, brush);
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
    QCOMPARE(d3, test);
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

void tst_QDataStream::writenullptr_t(QDataStream *s)
{
    *s << nullptr;
}

void tst_QDataStream::readQByteArray(QDataStream *s)
{
    QByteArray test(qByteArrayData(dataIndex(QTest::currentDataTag())));
    QByteArray d4;
    *s >> d4;
    QCOMPARE(d4, test);
}

void tst_QDataStream::readnullptr_t(QDataStream *s)
{
    std::nullptr_t ptr;
    *s >> ptr;
    QCOMPARE(ptr, nullptr);
}

// ************************************
#ifndef QT_NO_CURSOR
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

#ifndef QT_NO_CURSOR
void tst_QDataStream::stream_QCursor_data()
{
    stream_data(9);
}
#endif

#ifndef QT_NO_CURSOR
void tst_QDataStream::stream_QCursor()
{
    STREAM_IMPL(QCursor);
}
#endif

#ifndef QT_NO_CURSOR
void tst_QDataStream::writeQCursor(QDataStream *s)
{
    QCursor d5(qCursorData(dataIndex(QTest::currentDataTag())));
    *s << d5;
}
#endif

#ifndef QT_NO_CURSOR
void tst_QDataStream::readQCursor(QDataStream *s)
{
    QCursor test(qCursorData(dataIndex(QTest::currentDataTag())));
    QCursor d5;
    *s >> d5;

    QVERIFY(d5.shape() == test.shape()); //## lacks operator==
    QCOMPARE(d5.hotSpot(), test.hotSpot());

    // Comparing non-null QBitmaps will fail. Upcast them first to pass.
    QCOMPARE(d5.bitmap(Qt::ReturnByValue).isNull(), test.bitmap(Qt::ReturnByValue).isNull());
    QCOMPARE(
        static_cast<QPixmap>(d5.bitmap(Qt::ReturnByValue)),
        static_cast<QPixmap>(test.bitmap(Qt::ReturnByValue))
    );
    QCOMPARE(d5.mask(Qt::ReturnByValue).isNull(), test.mask(Qt::ReturnByValue).isNull());
    QCOMPARE(
        static_cast<QPixmap>(d5.mask(Qt::ReturnByValue)),
        static_cast<QPixmap>(test.mask(Qt::ReturnByValue))
    );
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
    QCOMPARE(d6, test);
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
    case 60: return QTime();
    }
    return QTime(0, 0, 0);
}
#define MAX_QTIME_DATA 61

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
    QCOMPARE(d7, test);
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

void tst_QDataStream::stream_nullptr_t_data()
{
    stream_data(1); // there's only one value possible
}

void tst_QDataStream::stream_nullptr_t()
{
    using namespace std;
    STREAM_IMPL(nullptr_t);
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
    QCOMPARE(d8, test);
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
    QCOMPARE(d12, ref);

    // do some extra neurotic tests
    QCOMPARE(d12.size(), ref.size());
    QCOMPARE(d12.isNull(), ref.isNull());
    QCOMPARE(d12.width(), ref.width());
    QCOMPARE(d12.height(), ref.height());
    QCOMPARE(d12.depth(), ref.depth());
    QCOMPARE(d12.colorCount(), ref.colorCount());
    QCOMPARE(d12.hasAlphaChannel(), ref.hasAlphaChannel());
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
    QCOMPARE(d15.capStyle(), origPen.capStyle());
    QCOMPARE(d15.joinStyle(), origPen.joinStyle());
    QCOMPARE(d15, origPen);
}

// ************************************

// pixmap testing is currently limited to one pixmap only.
//
void tst_QDataStream::stream_QPixmap_data()
{
    stream_data(1);
}

void tst_QDataStream::stream_QPixmap()
{
    STREAM_IMPL(QPixmap);
}

void tst_QDataStream::stream_QIcon_data()
{
    stream_data(1);
}

void tst_QDataStream::stream_QIcon()
{
    STREAM_IMPL(QIcon);
}

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
    QCOMPARE(d16.width(), pm.width());
    QCOMPARE(d16.height(), pm.height());
    QCOMPARE(d16.size(), pm.size());
    QCOMPARE(d16.rect(), pm.rect());
    QCOMPARE(d16.depth(), pm.depth());
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
    QCOMPARE(d17, ref);

    QPointF d17f;
    *s >> d17f;
    QCOMPARE(d17f, QPointF(ref));
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
    QCOMPARE(d18, ref);

    QRectF d18f;
    *s >> d18f;
    QCOMPARE(d18f, QRectF(ref));
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
    QCOMPARE(d19, ref);

    QPolygonF d19f;
    *s >> d19f;
    QCOMPARE(d19f, QPolygonF(ref));
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
#if !defined(Q_OS_UNIX) // all our Unix platforms use X regions.
    case 7: return QRegion(1, 2, 300, 400, QRegion::Ellipse);
    case 8: return QRegion(100, 100, 1024, 768, QRegion::Ellipse);
    case 9: return QRegion(-100, -100, 1024, 1024, QRegion::Ellipse);
    case 10: return QRegion(100, -100, 2048, 4096, QRegion::Ellipse);
    case 11: return QRegion(-100, 100, 4096, 2048, QRegion::Ellipse);
        // simplest X11 case that fails:
    case 12: return QRegion(0, 0, 3, 3, QRegion::Ellipse);
#else
    case 7:
        qWarning("Skipping streaming of elliptical regions on embedded, OS X, and X11;"
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
    QCOMPARE(r, ref);
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
    QCOMPARE(d21, ref);

    QSizeF d21f;
    *s >> d21f;
    QCOMPARE(d21f, QSizeF(ref));
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

void tst_QDataStream::stream_QJsonDocument()
{
    QByteArray buffer;
    {
        QDataStream save(&buffer, QIODevice::WriteOnly);
        save << QByteArrayLiteral("invalidJson");
        QDataStream load(&buffer, QIODevice::ReadOnly);
        QJsonDocument doc;
        load >> doc;
        QVERIFY(doc.isEmpty());
        QVERIFY(load.status() != QDataStream::Ok);
        QCOMPARE(load.status(), QDataStream::ReadCorruptData);
    }
    {
        QDataStream save(&buffer, QIODevice::WriteOnly);
        QJsonDocument docSave(QJsonArray{1,2,3});
        save << docSave;
        QDataStream load(&buffer, QIODevice::ReadOnly);
        QJsonDocument docLoad;
        load >> docLoad;
        QCOMPARE(docLoad, docSave);
    }
}

void tst_QDataStream::stream_QJsonArray()
{
    QByteArray buffer;
    {
        QDataStream save(&buffer, QIODevice::WriteOnly);
        save << QByteArrayLiteral("invalidJson");
        QDataStream load(&buffer, QIODevice::ReadOnly);
        QJsonArray array;
        load >> array;
        QVERIFY(array.isEmpty());
        QVERIFY(load.status() != QDataStream::Ok);
        QCOMPARE(load.status(), QDataStream::ReadCorruptData);
    }
    {
        QDataStream save(&buffer, QIODevice::WriteOnly);
        QJsonArray arraySave(QJsonArray{1,2,3});
        save << arraySave;
        QDataStream load(&buffer, QIODevice::ReadOnly);
        QJsonArray arrayLoad;
        load >> arrayLoad;
        QCOMPARE(arrayLoad, arraySave);
    }
}

void tst_QDataStream::stream_QJsonObject()
{
    QByteArray buffer;
    {
        QDataStream save(&buffer, QIODevice::WriteOnly);
        save << QByteArrayLiteral("invalidJson");
        QDataStream load(&buffer, QIODevice::ReadOnly);
        QJsonObject object;
        load >> object;
        QVERIFY(object.isEmpty());
        QVERIFY(load.status() != QDataStream::Ok);
        QCOMPARE(load.status(), QDataStream::ReadCorruptData);
    }
    {
        QDataStream save(&buffer, QIODevice::WriteOnly);
        QJsonObject objSave{{"foo", 1}, {"bar", 2}};
        save << objSave;
        QDataStream load(&buffer, QIODevice::ReadOnly);
        QJsonObject objLoad;
        load >> objLoad;
        QCOMPARE(objLoad, objSave);
    }
}

void tst_QDataStream::stream_QJsonValue()
{
    QByteArray buffer;
    {
        QDataStream save(&buffer, QIODevice::WriteOnly);
        save << quint8(42);
        QDataStream load(&buffer, QIODevice::ReadOnly);
        QJsonValue value;
        load >> value;
        QVERIFY(value.isUndefined());
        QVERIFY(load.status() != QDataStream::Ok);
        QCOMPARE(load.status(), QDataStream::ReadCorruptData);
    }
    {
        QDataStream save(&buffer, QIODevice::WriteOnly);
        QJsonValue valueSave{42};
        save << valueSave;
        QDataStream load(&buffer, QIODevice::ReadOnly);
        QJsonValue valueLoad;
        load >> valueLoad;
        QCOMPARE(valueLoad, valueSave);
    }
}

void tst_QDataStream::stream_QCborArray()
{
    QByteArray buffer;
    QDataStream save(&buffer, QIODevice::WriteOnly);
    QCborArray arraySave({1, 2, 3});
    save << arraySave;
    QDataStream load(&buffer, QIODevice::ReadOnly);
    QCborArray arrayLoad;
    load >> arrayLoad;
    QCOMPARE(arrayLoad, arraySave);
}

void tst_QDataStream::stream_QCborMap()
{
    QByteArray buffer;
    QDataStream save(&buffer, QIODevice::WriteOnly);
    QCborMap objSave{{"foo", 1}, {"bar", 2}};
    save << objSave;
    QDataStream load(&buffer, QIODevice::ReadOnly);
    QCborMap objLoad;
    load >> objLoad;
    QCOMPARE(objLoad, objSave);
}

void tst_QDataStream::stream_QCborValue()
{
    QByteArray buffer;
    QDataStream save(&buffer, QIODevice::WriteOnly);
    QCborValue valueSave{42};
    save << valueSave;
    QDataStream load(&buffer, QIODevice::ReadOnly);
    QCborValue valueLoad;
    load >> valueLoad;
    QCOMPARE(valueLoad, valueSave);
}

void tst_QDataStream::setVersion_data()
{
    QTest::addColumn<int>("vers");
    QDataStream latest;

    for (int vers = 1; vers <= latest.version(); ++vers)
        QTest::newRow(("v_" + QByteArray::number(vers)).constData()) << vers;
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
        QCOMPARE(keyseq1, QKeySequence(Qt::Key_A));
        if (vers >= 5) {
            QVERIFY(keyseq2 == QKeySequence(Qt::Key_B, Qt::Key_C));
        } else {
            QCOMPARE(keyseq2, QKeySequence(Qt::Key_B));
        }
        QCOMPARE(deadbeef, 0xDEADBEEF);
    }

    /*
        Test QPalette.
    */

    // revise the test if new color roles or color groups are added
    QVERIFY(QPalette::NColorRoles == QPalette::PlaceholderText + 1);
    QCOMPARE(int(QPalette::NColorGroups), 3);

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
                QVERIFY(pal1.color((QPalette::ColorGroup)grp, QPalette::WindowText)
                        == inPal1.color((QPalette::ColorGroup)grp, QPalette::WindowText));
                QVERIFY(pal1.color((QPalette::ColorGroup)grp, QPalette::Window)
                        == inPal1.color((QPalette::ColorGroup)grp, QPalette::Window));
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
            QCOMPARE(pal1, inPal1);
            QCOMPARE(pal2, inPal2);
        }
    }
}

class SequentialBuffer : public QIODevice
{
public:
    SequentialBuffer(QByteArray *data) : QIODevice() { buf.setBuffer(data); }

    bool isSequential() const override { return true; }
    bool open(OpenMode mode) override { return buf.open(mode) && QIODevice::open(mode | QIODevice::Unbuffered); }
    void close() override { buf.close(); QIODevice::close(); }
    qint64 bytesAvailable() const override { return QIODevice::bytesAvailable() + buf.bytesAvailable(); }

protected:
    qint64 readData(char *data, qint64 maxSize) override { return buf.read(data, maxSize); }
    qint64 writeData(const char *data, qint64 maxSize) override { return buf.write(data, maxSize); }

private:
    QBuffer buf;
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

    QByteArray oneMbMinus1(1024 * 1024 - 1, '\0');
    for (int i = 0; i < oneMbMinus1.size(); ++i)
        oneMbMinus1[i] = 0x1 | (8 * ((uchar)i / 9));
    QByteArray threeMbMinus1 = oneMbMinus1 + 'j' + oneMbMinus1 + 'k' + oneMbMinus1;

    // ok
    QTest::newRow("size 0") << QByteArray("\x00\x00\x00\x00", 4) << (int) QDataStream::Ok << QByteArray();
    QTest::newRow("size 1") << QByteArray("\x00\x00\x00\x01j", 5) << (int) QDataStream::Ok << QByteArray("j");
    QTest::newRow("size 2") << QByteArray("\x00\x00\x00\x02jk", 6) << (int) QDataStream::Ok << QByteArray("jk");
    QTest::newRow("size 3") << QByteArray("\x00\x00\x00\x03jkl", 7) << (int) QDataStream::Ok << QByteArray("jkl");
    QTest::newRow("size 4") << QByteArray("\x00\x00\x00\x04jklm", 8) << (int) QDataStream::Ok << QByteArray("jklm");
    QTest::newRow("size 4j") << QByteArray("\x00\x00\x00\x04jklmj", 8) << (int) QDataStream::Ok << QByteArray("jklm");
    QTest::newRow("size 1MB-1") << QByteArray("\x00\x0f\xff\xff", 4) + oneMbMinus1 + QByteArray("j") << (int) QDataStream::Ok << oneMbMinus1;
    QTest::newRow("size 1MB") << QByteArray("\x00\x10\x00\x00", 4) + oneMbMinus1 + QByteArray("jkl") << (int) QDataStream::Ok << oneMbMinus1 + "j";
    QTest::newRow("size 1MB+1") << QByteArray("\x00\x10\x00\x01", 4) + oneMbMinus1 + QByteArray("jkl") << (int) QDataStream::Ok << oneMbMinus1 + "jk";
    QTest::newRow("size 3MB-1") << QByteArray("\x00\x2f\xff\xff", 4) + threeMbMinus1 + QByteArray("j") << (int) QDataStream::Ok << threeMbMinus1;
    QTest::newRow("size 3MB") << QByteArray("\x00\x30\x00\x00", 4) + threeMbMinus1 + QByteArray("jkl") << (int) QDataStream::Ok << threeMbMinus1 + "j";
    QTest::newRow("size 3MB+1") << QByteArray("\x00\x30\x00\x01", 4) + threeMbMinus1 + QByteArray("jkl") << (int) QDataStream::Ok << threeMbMinus1 + "jk";

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
    QTest::newRow("badsize 1MB") << QByteArray("\x00\x10\x00\x00", 4) + oneMbMinus1 << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("badsize 1MB+1") << QByteArray("\x00\x10\x00\x01", 4) + oneMbMinus1 + QByteArray("j") << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("badsize 3MB") << QByteArray("\x00\x30\x00\x00", 4) + threeMbMinus1 << (int) QDataStream::ReadPastEnd << QByteArray();
    QTest::newRow("badsize 3MB+1") << QByteArray("\x00\x30\x00\x01", 4) + threeMbMinus1 + QByteArray("j") << (int) QDataStream::ReadPastEnd << QByteArray();
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

    QString oneMbMinus1;
    oneMbMinus1.resize(1024 * 1024 - 1);
    for (int i = 0; i < oneMbMinus1.size(); ++i)
        oneMbMinus1[i] = 0x1 | (8 * ((uchar)i / 9));
    QString threeMbMinus1 = oneMbMinus1 + QChar('j') + oneMbMinus1 + QChar('k') + oneMbMinus1;

    QByteArray threeMbMinus1Data = qstring2qbytearray(threeMbMinus1);
    QByteArray oneMbMinus1Data = qstring2qbytearray(oneMbMinus1);

    // ok
    QTest::newRow("size 0") << QByteArray("\x00\x00\x00\x00", 4) << (int) QDataStream::Ok << QString();
    QTest::newRow("size 1") << QByteArray("\x00\x00\x00\x02\x00j", 6) << (int) QDataStream::Ok << QString("j");
    QTest::newRow("size 2") << QByteArray("\x00\x00\x00\x04\x00j\x00k", 8) << (int) QDataStream::Ok << QString("jk");
    QTest::newRow("size 3") << QByteArray("\x00\x00\x00\x06\x00j\x00k\x00l", 10) << (int) QDataStream::Ok << QString("jkl");
    QTest::newRow("size 4") << QByteArray("\x00\x00\x00\x08\x00j\x00k\x00l\x00m", 12) << (int) QDataStream::Ok << QString("jklm");
    QTest::newRow("size 4j") << QByteArray("\x00\x00\x00\x08\x00j\x00k\x00l\x00mjj", 14) << (int) QDataStream::Ok << QString("jklm");
    QTest::newRow("size 1MB-1") << QByteArray("\x00\x1f\xff\xfe", 4) + oneMbMinus1Data + QByteArray("jj") << (int) QDataStream::Ok << oneMbMinus1;
    QTest::newRow("size 1MB") << QByteArray("\x00\x20\x00\x00", 4) + oneMbMinus1Data + QByteArray("\x00j\x00k\x00l", 6) << (int) QDataStream::Ok << oneMbMinus1 + "j";
    QTest::newRow("size 1MB+1") << QByteArray("\x00\x20\x00\x02", 4) + oneMbMinus1Data + QByteArray("\x00j\x00k\x00l", 6) << (int) QDataStream::Ok << oneMbMinus1 + "jk";
    QTest::newRow("size 3MB-1") << QByteArray("\x00\x5f\xff\xfe", 4) + threeMbMinus1Data + QByteArray("jj") << (int) QDataStream::Ok << threeMbMinus1;
    QTest::newRow("size 3MB") << QByteArray("\x00\x60\x00\x00", 4) + threeMbMinus1Data + QByteArray("\x00j\x00k\x00l", 6) << (int) QDataStream::Ok << threeMbMinus1 + "j";
    QTest::newRow("size 3MB+1") << QByteArray("\x00\x60\x00\x02", 4) + threeMbMinus1Data + QByteArray("\x00j\x00k\x00l", 6) << (int) QDataStream::Ok << threeMbMinus1 + "jk";

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
    QTest::newRow("badsize 1MB") << QByteArray("\x00\x20\x00\x00", 4) + oneMbMinus1Data << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("badsize 1MB+1") << QByteArray("\x00\x20\x00\x02", 4) + oneMbMinus1Data + QByteArray("j") << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("badsize 3MB") << QByteArray("\x00\x60\x00\x00", 4) + threeMbMinus1Data << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("badsize 3MB+1") << QByteArray("\x00\x60\x00\x02", 4) + threeMbMinus1Data + QByteArray("j") << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("size -2") << QByteArray("\xff\xff\xff\xfe", 4) << (int) QDataStream::ReadPastEnd << QString();
    QTest::newRow("size MAX") << QByteArray("\x7f\xff\xff\xfe", 4) << (int) QDataStream::ReadPastEnd << QString();

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

#define MAP_TEST(byteArray, initialStatus, expectedStatus, expectedHash) \
    for (bool inTransaction = false;; inTransaction = true) { \
        { \
            QByteArray ba = byteArray; \
            QDataStream stream(&ba, QIODevice::ReadOnly); \
            if (inTransaction) \
                stream.startTransaction(); \
            stream.setStatus(initialStatus); \
            stream >> hash; \
            QCOMPARE((int)stream.status(), (int)expectedStatus); \
            if (!inTransaction || stream.commitTransaction()) { \
                QCOMPARE(hash.size(), expectedHash.size()); \
                QCOMPARE(hash, expectedHash); \
            } else { \
                QVERIFY(hash.isEmpty()); \
            } \
        } \
        { \
            QByteArray ba = byteArray; \
            StringMap expectedMap; \
            StringHash::const_iterator it = expectedHash.constBegin(); \
            for (; it != expectedHash.constEnd(); ++it) \
                expectedMap.insert(it.key(), it.value()); \
            QDataStream stream(&ba, QIODevice::ReadOnly); \
            if (inTransaction) \
                stream.startTransaction(); \
            stream.setStatus(initialStatus); \
            stream >> map; \
            QCOMPARE((int)stream.status(), (int)expectedStatus); \
            if (!inTransaction || stream.commitTransaction()) { \
                QCOMPARE(map.size(), expectedMap.size()); \
                QCOMPARE(map, expectedMap); \
            } else { \
                QVERIFY(map.isEmpty()); \
            } \
        } \
        if (inTransaction) \
            break; \
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
    MAP_TEST(QByteArray("\x00\x00\x00\x00", 4), QDataStream::Ok, QDataStream::Ok, StringHash());
    MAP_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00", 12), QDataStream::Ok, QDataStream::Ok, hash1);
    MAP_TEST(QByteArray("\x00\x00\x00\x02\x00\x00\x00\x02\x00J\x00\x00\x00\x02\x00K"
                        "\x00\x00\x00\x02\x00L\x00\x00\x00\x04\x00M\x00N", 30), QDataStream::Ok, QDataStream::Ok, hash2);

    // past end
    MAP_TEST(QByteArray(), QDataStream::Ok, QDataStream::ReadPastEnd, StringHash());
    MAP_TEST(QByteArray("\x00", 1), QDataStream::Ok, QDataStream::ReadPastEnd, StringHash());
    MAP_TEST(QByteArray("\x00\x00", 2), QDataStream::Ok, QDataStream::ReadPastEnd, StringHash());
    MAP_TEST(QByteArray("\x00\x00\x00", 3), QDataStream::Ok, QDataStream::ReadPastEnd, StringHash());
    MAP_TEST(QByteArray("\x00\x00\x00\x01", 4), QDataStream::Ok, QDataStream::ReadPastEnd, StringHash());
    for (int i = 4; i < 12; ++i) {
        MAP_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00", i), QDataStream::Ok, QDataStream::ReadPastEnd, StringHash());
    }

    // corrupt data
    MAP_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x01", 8), QDataStream::Ok, QDataStream::ReadCorruptData, StringHash());
    MAP_TEST(QByteArray("\x00\x00\x00\x02\x00\x00\x00\x01\x00J\x00\x00\x00\x01\x00K"
                        "\x00\x00\x00\x01\x00L\x00\x00\x00\x02\x00M\x00N", 30), QDataStream::Ok, QDataStream::ReadCorruptData, StringHash());

    // test the previously latched error status is not affected by reading
    MAP_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x00\x00\x00\x00\x00", 12), QDataStream::ReadPastEnd, QDataStream::ReadPastEnd, hash1);
    MAP_TEST(QByteArray("\x00\x00\x00\x01", 4), QDataStream::ReadCorruptData, QDataStream::ReadCorruptData, StringHash());
    MAP_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x01", 8), QDataStream::ReadPastEnd, QDataStream::ReadPastEnd, StringHash());
}

#define LIST_TEST(byteArray, initialStatus, expectedStatus, expectedList) \
    for (bool inTransaction = false;; inTransaction = true) { \
        { \
            QByteArray ba = byteArray; \
            QDataStream stream(&ba, QIODevice::ReadOnly); \
            if (inTransaction) \
                stream.startTransaction(); \
            stream.setStatus(initialStatus); \
            stream >> list; \
            QCOMPARE((int)stream.status(), (int)expectedStatus); \
            if (!inTransaction || stream.commitTransaction()) { \
                QCOMPARE(list.size(), expectedList.size()); \
                QCOMPARE(list, expectedList); \
            } else { \
                QVERIFY(list.isEmpty()); \
            } \
        } \
        { \
            Vector expectedVector; \
            for (int i = 0; i < expectedList.count(); ++i) \
                expectedVector << expectedList.at(i); \
            QByteArray ba = byteArray; \
            QDataStream stream(&ba, QIODevice::ReadOnly); \
            if (inTransaction) \
                stream.startTransaction(); \
            stream.setStatus(initialStatus); \
            stream >> vector; \
            QCOMPARE((int)stream.status(), (int)expectedStatus); \
            if (!inTransaction || stream.commitTransaction()) { \
                QCOMPARE(vector.size(), expectedVector.size()); \
                QCOMPARE(vector, expectedVector); \
            } else { \
                QVERIFY(vector.isEmpty()); \
            } \
        } \
        if (inTransaction) \
            break; \
    }

#define LINKED_LIST_TEST(byteArray, initialStatus, expectedStatus, expectedList) \
    for (bool inTransaction = false;; inTransaction = true) { \
        { \
            LinkedList expectedLinkedList; \
            for (int i = 0; i < expectedList.count(); ++i) \
                expectedLinkedList << expectedList.at(i); \
            QByteArray ba = byteArray; \
            QDataStream stream(&ba, QIODevice::ReadOnly); \
            if (inTransaction) \
                stream.startTransaction(); \
            stream.setStatus(initialStatus); \
            stream >> linkedList; \
            QCOMPARE((int)stream.status(), (int)expectedStatus); \
            if (!inTransaction || stream.commitTransaction()) { \
                QCOMPARE(linkedList.size(), expectedLinkedList.size()); \
                QCOMPARE(linkedList, expectedLinkedList); \
            } else { \
                QVERIFY(linkedList.isEmpty()); \
            } \
        } \
        if (inTransaction) \
            break; \
    }

void tst_QDataStream::status_QLinkedList_QList_QVector()
{
    typedef QList<QString> List;
    typedef QVector<QString> Vector;
    List list;
    Vector vector;

    // ok
    {
        List listWithEmptyString;
        listWithEmptyString.append("");

        List someList;
        someList.append("J");
        someList.append("MN");

        LIST_TEST(QByteArray("\x00\x00\x00\x00", 4), QDataStream::Ok, QDataStream::Ok, List());
        LIST_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x00", 8), QDataStream::Ok, QDataStream::Ok, listWithEmptyString);
        LIST_TEST(QByteArray("\x00\x00\x00\x02\x00\x00\x00\x02\x00J"
                             "\x00\x00\x00\x04\x00M\x00N", 18), QDataStream::Ok, QDataStream::Ok, someList);
    }

    // past end
    {
        LIST_TEST(QByteArray(), QDataStream::Ok, QDataStream::ReadPastEnd, List());
        LIST_TEST(QByteArray("\x00", 1), QDataStream::Ok, QDataStream::ReadPastEnd, List());
        LIST_TEST(QByteArray("\x00\x00", 2), QDataStream::Ok, QDataStream::ReadPastEnd, List());
        LIST_TEST(QByteArray("\x00\x00\x00", 3), QDataStream::Ok, QDataStream::ReadPastEnd, List());
        LIST_TEST(QByteArray("\x00\x00\x00\x01", 4), QDataStream::Ok, QDataStream::ReadPastEnd, List());
        for (int i = 4; i < 12; ++i) {
            LIST_TEST(QByteArray("\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00", i), QDataStream::Ok, QDataStream::ReadPastEnd, List());
        }
    }

    // corrupt data
    {
        LIST_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x01", 8), QDataStream::Ok, QDataStream::ReadCorruptData, List());
        LIST_TEST(QByteArray("\x00\x00\x00\x02\x00\x00\x00\x01\x00J"
                             "\x00\x00\x00\x02\x00M\x00N", 18), QDataStream::Ok, QDataStream::ReadCorruptData, List());
    }

    // test the previously latched error status is not affected by reading
    {
        List listWithEmptyString;
        listWithEmptyString.append("");

        LIST_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x00", 8), QDataStream::ReadPastEnd, QDataStream::ReadPastEnd, listWithEmptyString);
        LIST_TEST(QByteArray("\x00\x00\x00\x01", 4), QDataStream::ReadCorruptData, QDataStream::ReadCorruptData, List());
        LIST_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x01", 8), QDataStream::ReadPastEnd, QDataStream::ReadPastEnd, List());
    }

#if QT_DEPRECATED_SINCE(5, 15)
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
    // The same as above with QLinkedList

    typedef QLinkedList<QString> LinkedList;
    LinkedList linkedList;

    // ok
    {
        List listWithEmptyString;
        listWithEmptyString.append("");

        List someList;
        someList.append("J");
        someList.append("MN");

        LINKED_LIST_TEST(QByteArray("\x00\x00\x00\x00", 4), QDataStream::Ok, QDataStream::Ok, List());
        LINKED_LIST_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x00", 8), QDataStream::Ok, QDataStream::Ok, listWithEmptyString);
        LINKED_LIST_TEST(QByteArray("\x00\x00\x00\x02\x00\x00\x00\x02\x00J"
                                    "\x00\x00\x00\x04\x00M\x00N", 18), QDataStream::Ok, QDataStream::Ok, someList);
    }

    // past end
    {
        LINKED_LIST_TEST(QByteArray(), QDataStream::Ok, QDataStream::ReadPastEnd, List());
        LINKED_LIST_TEST(QByteArray("\x00", 1), QDataStream::Ok, QDataStream::ReadPastEnd, List());
        LINKED_LIST_TEST(QByteArray("\x00\x00", 2), QDataStream::Ok, QDataStream::ReadPastEnd, List());
        LINKED_LIST_TEST(QByteArray("\x00\x00\x00", 3), QDataStream::Ok, QDataStream::ReadPastEnd, List());
        LINKED_LIST_TEST(QByteArray("\x00\x00\x00\x01", 4), QDataStream::Ok, QDataStream::ReadPastEnd, List());
        for (int i = 4; i < 12; ++i) {
            LINKED_LIST_TEST(QByteArray("\x00\x00\x00\x02\x00\x00\x00\x00\x00\x00\x00\x00", i), QDataStream::Ok, QDataStream::ReadPastEnd, List());
        }
    }

    // corrupt data
    {
        LINKED_LIST_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x01", 8), QDataStream::Ok, QDataStream::ReadCorruptData, List());
        LINKED_LIST_TEST(QByteArray("\x00\x00\x00\x02\x00\x00\x00\x01\x00J"
                                    "\x00\x00\x00\x02\x00M\x00N", 18), QDataStream::Ok, QDataStream::ReadCorruptData, List());
    }

    // test the previously latched error status is not affected by reading
    {
        List listWithEmptyString;
        listWithEmptyString.append("");

        LINKED_LIST_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x00", 8), QDataStream::ReadPastEnd, QDataStream::ReadPastEnd, listWithEmptyString);
        LINKED_LIST_TEST(QByteArray("\x00\x00\x00\x01", 4), QDataStream::ReadCorruptData, QDataStream::ReadCorruptData, List());
        LINKED_LIST_TEST(QByteArray("\x00\x00\x00\x01\x00\x00\x00\x01", 8), QDataStream::ReadPastEnd, QDataStream::ReadPastEnd, List());
    }

QT_WARNING_POP
#endif
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
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
        QCOMPARE(rGrad.matrix(), radialBrush.matrix());
QT_WARNING_POP
        QCOMPARE(rGrad.gradient()->type(), radialBrush.gradient()->type());
        QCOMPARE(rGrad.gradient()->stops(), radialBrush.gradient()->stops());
        QCOMPARE(rGrad.gradient()->spread(), radialBrush.gradient()->spread());
        QCOMPARE(((QRadialGradient *)rGrad.gradient())->center(), ((QRadialGradient *)radialBrush.gradient())->center());
        QCOMPARE(((QRadialGradient *)rGrad.gradient())->focalPoint(), ((QRadialGradient *)radialBrush.gradient())->focalPoint());
        QCOMPARE(((QRadialGradient *)rGrad.gradient())->radius(), ((QRadialGradient *)radialBrush.gradient())->radius());
        stream >> cGrad;
        QCOMPARE(cGrad.style(), conicalBrush.style());
QT_WARNING_PUSH
QT_WARNING_DISABLE_DEPRECATED
        QCOMPARE(cGrad.matrix(), conicalBrush.matrix());
QT_WARNING_POP
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

void tst_QDataStream::compatibility_Qt5()
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
        out.setVersion(QDataStream::Qt_5_7);
        out << palette;
        out << brush;
    }
    QBrush in_brush;
    QPalette in_palette;
    {
        QDataStream in(stream);
        in.setVersion(QDataStream::Qt_5_7);
        in >> in_palette;
        in >> in_brush;
    }
    QCOMPARE(in_brush.style(), Qt::LinearGradientPattern);
    QCOMPARE(in_palette.brush(QPalette::Button).style(), Qt::LinearGradientPattern);
    QCOMPARE(in_palette.color(QPalette::Light), QColor(Qt::green));
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
        QCOMPARE(in_brush.style(), Qt::NoBrush);
        QCOMPARE(in_palette.brush(QPalette::Button).style(), Qt::NoBrush);
        QCOMPARE(in_palette.color(QPalette::Light), QColor(Qt::green));
    }
    // QTime() was serialized to (0, 0, 0, 0) in Qt3, not (0xFF, 0xFF, 0xFF, 0xFF)
    // This is because in Qt3 a null time was valid, and there was no support for deserializing a value of -1.
    {
        QByteArray stream;
        {
            QDataStream out(&stream, QIODevice::WriteOnly);
            out.setVersion(QDataStream::Qt_3_3);
            out << QTime();
        }
        QTime in_time;
        {
            QDataStream in(stream);
            in.setVersion(QDataStream::Qt_3_3);
            in >> in_time;
        }
        QVERIFY(in_time.isNull());

        quint32 rawValue;
        QDataStream in(stream);
        in.setVersion(QDataStream::Qt_3_3);
        in >> rawValue;
        QCOMPARE(rawValue, quint32(0));
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
    QCOMPARE(in_brush.style(), Qt::NoBrush);
    QCOMPARE(in_palette.brush(QPalette::Button).style(), Qt::NoBrush);
    QCOMPARE(in_palette.color(QPalette::Light), QColor(Qt::green));
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

void tst_QDataStream::enumTest()
{
    QByteArray ba;

    enum class E1 : qint8
    {
        A,
        B,
        C
    };
    {
        QDataStream stream(&ba, QIODevice::WriteOnly);
        stream << E1::A;
        QCOMPARE(ba.size(), int(sizeof(E1)));
    }
    {
        QDataStream stream(ba);
        E1 e;
        stream >> e;
        QCOMPARE(e, E1::A);
    }
    ba.clear();

    enum class E2 : qint16
    {
        A,
        B,
        C
    };
    {
        QDataStream stream(&ba, QIODevice::WriteOnly);
        stream << E2::B;
        QCOMPARE(ba.size(), int(sizeof(E2)));
    }
    {
        QDataStream stream(ba);
        E2 e;
        stream >> e;
        QCOMPARE(e, E2::B);
    }
    ba.clear();

    enum class E4 : qint32
    {
        A,
        B,
        C
    };
    {
        QDataStream stream(&ba, QIODevice::WriteOnly);
        stream << E4::C;
        QCOMPARE(ba.size(), int(sizeof(E4)));
    }
    {
        QDataStream stream(ba);
        E4 e;
        stream >> e;
        QCOMPARE(e, E4::C);
    }
    ba.clear();


    enum E
    {
        A,
        B,
        C,
        D
    };
    {
        QDataStream stream(&ba, QIODevice::WriteOnly);
        stream << E::D;
        QCOMPARE(ba.size(), 4);
    }
    {
        QDataStream stream(ba);
        E e;
        stream >> e;
        QCOMPARE(e, E::D);
    }
    ba.clear();

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

void tst_QDataStream::transaction_data()
{
    QTest::addColumn<qint8>("i8Data");
    QTest::addColumn<qint16>("i16Data");
    QTest::addColumn<qint32>("i32Data");
    QTest::addColumn<qint64>("i64Data");
    QTest::addColumn<bool>("bData");
    QTest::addColumn<float>("fData");
    QTest::addColumn<double>("dData");
    QTest::addColumn<QImage>("imgData");
    QTest::addColumn<QByteArray>("strData");
    QTest::addColumn<QByteArray>("rawData");

    QImage img1(open_xpm);
    QImage img2;
    QImage img3(50, 50, QImage::Format_ARGB32);
    img3.fill(qRgba(12, 34, 56, 78));

    QTest::newRow("1") << qint8(1) << qint16(2) << qint32(3) << qint64(4) << true << 5.0f
                       << double(6.0) << img1 << QByteArray("Hello world!") << QByteArray("Qt rocks!");
    QTest::newRow("2") << qint8(1 << 6) << qint16(1 << 14) << qint32(1 << 30) << qint64Data(3) << false << 123.0f
                       << double(234.0) << img2 << stringData(5).toUtf8() << stringData(6).toUtf8();
    QTest::newRow("3") << qint8(-1) << qint16(-2) << qint32(-3) << qint64(-4) << true << -123.0f
                       << double(-234.0) << img3 << stringData(3).toUtf8() << stringData(4).toUtf8();
}

void tst_QDataStream::transaction()
{
    QByteArray testBuffer;

    QFETCH(qint8, i8Data);
    QFETCH(qint16, i16Data);
    QFETCH(qint32, i32Data);
    QFETCH(qint64, i64Data);
    QFETCH(bool, bData);
    QFETCH(float, fData);
    QFETCH(double, dData);
    QFETCH(QImage, imgData);
    QFETCH(QByteArray, strData);
    QFETCH(QByteArray, rawData);

    {
        QDataStream stream(&testBuffer, QIODevice::WriteOnly);

        stream << i8Data << i16Data << i32Data << i64Data
               << bData << fData << dData << imgData << strData.constData();
        stream.writeRawData(rawData.constData(), rawData.size());
    }

    for (int splitPos = 0; splitPos <= testBuffer.size(); ++splitPos) {
        QByteArray readBuffer(testBuffer.left(splitPos));

        SequentialBuffer dev(&readBuffer);
        dev.open(QIODevice::ReadOnly);
        QDataStream stream(&dev);

        qint8 i8;
        qint16 i16;
        qint32 i32;
        qint64 i64;
        bool b;
        float f;
        double d;
        QImage img;
        char *str;
        QByteArray raw(rawData.size(), 0);

        forever {
            stream.startTransaction();
            stream >> i8 >> i16 >> i32 >> i64 >> b >> f >> d >> img >> str;
            stream.readRawData(raw.data(), raw.size());

            if (stream.commitTransaction())
                break;

            QVERIFY(stream.status() == QDataStream::ReadPastEnd);
            QVERIFY(splitPos == 0 || !stream.atEnd());
            QVERIFY(readBuffer.size() < testBuffer.size());
            delete [] str;
            raw.fill(0);
            readBuffer.append(testBuffer.right(testBuffer.size() - splitPos));
        }

        QVERIFY(stream.atEnd());
        QCOMPARE(i8, i8Data);
        QCOMPARE(i16, i16Data);
        QCOMPARE(i32, i32Data);
        QCOMPARE(i64, i64Data);
        QCOMPARE(b, bData);
        QCOMPARE(f, fData);
        QCOMPARE(d, dData);
        QCOMPARE(img, imgData);
        QVERIFY(strData == str);
        delete [] str;
        QCOMPARE(raw, rawData);
    }
}

void tst_QDataStream::nestedTransactionsResult_data()
{
    QTest::addColumn<bool>("commitFirst");
    QTest::addColumn<bool>("rollbackFirst");
    QTest::addColumn<bool>("commitSecond");
    QTest::addColumn<bool>("rollbackSecond");
    QTest::addColumn<bool>("successExpected");
    QTest::addColumn<bool>("expectedAtEnd");
    QTest::addColumn<int>("expectedStatus");

    QTest::newRow("1") << false << false << false << false
                       << false << true << int(QDataStream::ReadCorruptData);
    QTest::newRow("2") << false << false << false << true
                       << false << true << int(QDataStream::ReadCorruptData);
    QTest::newRow("3") << false << false << true << false
                       << false << true << int(QDataStream::ReadCorruptData);

    QTest::newRow("4") << false << true << false << false
                       << false << true << int(QDataStream::ReadCorruptData);
    QTest::newRow("5") << false << true << false << true
                       << false << false << int(QDataStream::ReadPastEnd);
    QTest::newRow("6") << false << true << true << false
                       << false << false << int(QDataStream::ReadPastEnd);

    QTest::newRow("7") << true << false << false << false
                       << false << true << int(QDataStream::ReadCorruptData);
    QTest::newRow("8") << true << false << false << true
                       << false << false << int(QDataStream::ReadPastEnd);
    QTest::newRow("9") << true << false << true << false
                       << true << true << int(QDataStream::Ok);
}

void tst_QDataStream::nestedTransactionsResult()
{
    QByteArray testBuffer(1, 0);
    QDataStream stream(&testBuffer, QIODevice::ReadOnly);
    uchar c;

    QFETCH(bool, commitFirst);
    QFETCH(bool, rollbackFirst);
    QFETCH(bool, commitSecond);
    QFETCH(bool, rollbackSecond);
    QFETCH(bool, successExpected);
    QFETCH(bool, expectedAtEnd);
    QFETCH(int, expectedStatus);

    stream.startTransaction();
    stream.startTransaction();
    stream >> c;

    if (commitFirst)
        QVERIFY(stream.commitTransaction());
    else if (rollbackFirst)
        stream.rollbackTransaction();
    else
        stream.abortTransaction();

    stream.startTransaction();

    if (commitSecond)
        QCOMPARE(stream.commitTransaction(), commitFirst);
    else if (rollbackSecond)
        stream.rollbackTransaction();
    else
        stream.abortTransaction();

    QCOMPARE(stream.commitTransaction(), successExpected);
    QCOMPARE(stream.atEnd(), expectedAtEnd);
    QCOMPARE(int(stream.status()), expectedStatus);
}

QTEST_MAIN(tst_QDataStream)
#include "tst_qdatastream.moc"

