// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0


#include <QtCore/QCoreApplication>
#include <QtCore/QtDebug>

#include <QTest>
#include <QtConcurrentRun>
#include <QFutureSynchronizer>
#include <QVariant>
#include <QSemaphore>
#include <QLine>
#include <QMimeType>
#include <QMimeDatabase>
#include <QMetaType>

#include <q20chrono.h>

#ifdef __cpp_lib_memory_resource
# include <memory_resource>
namespace pmr = std::pmr;
#else
namespace pmr = std;
#endif

using namespace std::chrono;
using namespace q20::chrono;
using namespace Qt::StringLiterals;

static_assert(QTypeTraits::has_ostream_operator_v<QDebug, int>);
static_assert(QTypeTraits::has_ostream_operator_v<QDebug, QMetaType>);
static_assert(QTypeTraits::has_ostream_operator_v<QDebug, QList<int>>);
static_assert(QTypeTraits::has_ostream_operator_v<QDebug, QMap<int, QString>>);
struct NonStreamable {};
static_assert(!QTypeTraits::has_ostream_operator_v<QDebug, NonStreamable>);
static_assert(!QTypeTraits::has_ostream_operator_v<QDebug, QList<NonStreamable>>);
static_assert(!QTypeTraits::has_ostream_operator_v<QDebug, QMap<int, NonStreamable>>);
struct ConvertsToQVariant {
    operator QVariant() {return QVariant::fromValue(*this);};
};
static_assert(!QTypeTraits::has_ostream_operator_v<QDebug, ConvertsToQVariant>);

#if defined(Q_OS_DARWIN)
#include <objc/runtime.h>
#include <Foundation/Foundation.h>
#endif

class tst_QDebug: public QObject
{
    Q_OBJECT
public:
    enum EnumType { EnumValue1 = 1, EnumValue2 = 2 };
    enum FlagType { EnumFlag1 = 1, EnumFlag2 = 2 };
    Q_ENUM(EnumType)
    Q_DECLARE_FLAGS(Flags, FlagType)
    Q_FLAG(Flags)

private slots:
    void assignment() const;
    void warningWithoutDebug() const;
    void criticalWithoutDebug() const;
    void basics() const;
    void debugWithBool() const;
    void debugSpaceHandling() const;
    void debugNoQuotes() const;
    void verbosity() const;
    void stateSaver() const;
    void veryLongWarningMessage() const;
    void qDebugQChar() const;
    void qDebugQMetaType() const;
    void qDebugQString() const;
    void qDebugQStringView() const;
    void qDebugQUtf8StringView() const;
    void qDebugQLatin1String() const;
    void qDebugStdString() const;
    void qDebugStdStringView() const;
    void qDebugStdWString() const;
    void qDebugStdWStringView() const;
    void qDebugStdU8String() const;
    void qDebugStdU8StringView() const;
    void qDebugStdU16String() const;
    void qDebugStdU16StringView() const;
    void qDebugStdU32String() const;
    void qDebugStdU32StringView() const;
    void qDebugQByteArray() const;
    void qDebugQByteArrayView() const;
    void qDebugQFlags() const;
    void qDebugStdChrono_data() const;
    void qDebugStdChrono() const;
    void textStreamModifiers() const;
    void resetFormat() const;
    void defaultMessagehandler() const;
    void threadSafety() const;
    void toString() const;
    void noQVariantEndlessRecursion() const;
#if defined(Q_OS_DARWIN)
    void objcInCppMode_data() const;
    void objcInCppMode() const;
    void objcInObjcMode_data() const;
    void objcInObjcMode() const;
#endif
};

void tst_QDebug::assignment() const
{
    QDebug debug1(QtDebugMsg);
    QDebug debug2(QtWarningMsg);

    QTest::ignoreMessage(QtDebugMsg, "foo");
    QTest::ignoreMessage(QtWarningMsg, "bar 1 2");

    debug1 << "foo";
    debug2 << "bar";
    debug1 = debug2;
    debug1 << "1";
    debug2 << "2";
}

static QtMsgType s_msgType;
static QString s_msg;
static QByteArray s_file;
static int s_line;
static QByteArray s_function;

static void myMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    s_msg = msg;
    s_msgType = type;
    s_file = context.file;
    s_line = context.line;
    s_function = context.function;
}

// Helper class to ensure that the testlib message handler gets
// restored at the end of each test function, even if the test
// fails or throws an exception.
class MessageHandlerSetter
{
public:
    MessageHandlerSetter(QtMessageHandler newMessageHandler)
        : oldMessageHandler(qInstallMessageHandler(newMessageHandler))
    { }

    ~MessageHandlerSetter()
    {
        qInstallMessageHandler(oldMessageHandler);
    }

private:
    QtMessageHandler oldMessageHandler;
};

/*! \internal
  The qWarning() stream should be usable even if QT_NO_DEBUG is defined.
 */
void tst_QDebug::warningWithoutDebug() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    { qWarning() << "A qWarning() message"; }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 2; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtWarningMsg);
    QCOMPARE(s_msg, QString::fromLatin1("A qWarning() message"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);
}

/*! \internal
  The qCritical() stream should be usable even if QT_NO_DEBUG is defined.
 */
void tst_QDebug::criticalWithoutDebug() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    { qCritical() << "A qCritical() message"; }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 2; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtCriticalMsg);
    QCOMPARE(s_msg, QString::fromLatin1("A qCritical() message"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);
}

void tst_QDebug::basics() const
{
    // test simple types, without quoting or other modifications
    // (bool tested in the next function)
    MessageHandlerSetter mhs(myMessageHandler);

    qDebug() << 'X';
    QCOMPARE(s_msg, "X");

    qDebug() << 123;
    QCOMPARE(s_msg, "123");

    qDebug() << 456U;
    QCOMPARE(s_msg, "456");

    qDebug() << -123L;
    QCOMPARE(s_msg, "-123");

    qDebug() << 456UL;
    QCOMPARE(s_msg, "456");

    qDebug() << Q_INT64_C(-123);
    QCOMPARE(s_msg, "-123");

    qDebug() << Q_UINT64_C(456);
    QCOMPARE(s_msg, "456");

    qDebug() << "Hello";
    QCOMPARE(s_msg, "Hello");

    qDebug() << u"World";
    QCOMPARE(s_msg, "World");

    qDebug() << (void *)0xfff;
    QCOMPARE(s_msg, "0xfff");

    qDebug() << nullptr;
    QCOMPARE(s_msg, "(nullptr)");
}

void tst_QDebug::debugWithBool() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    { qDebug() << false << true; }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 2; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("false true"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);
}

class MyPoint
{
public:
    MyPoint(int val1, int val2)
        : v1(val1), v2(val2) {}
    int v1;
    int v2;
};
QDebug operator<< (QDebug s, const MyPoint& point)
{
    const QDebugStateSaver saver(s);
    s.nospace() << "MyPoint(" << point.v1 << ", " << point.v2 << ")";
    return s;
}

class MyLine
{
public:
    MyLine(const MyPoint& point1, const MyPoint& point2)
        : p1(point1), p2(point2) {}
    MyPoint p1;
    MyPoint p2;
};
QDebug operator<< (QDebug s, const MyLine& line)
{
    const QDebugStateSaver saver(s);
    s.nospace();
    s << "MyLine(" << line.p1 << ", "<< line.p2;
    if (s.verbosity() > 2)
        s << ", Manhattan length=" << (qAbs(line.p2.v1 - line.p1.v1) + qAbs(line.p2.v2 - line.p1.v2));
    s << ')';
    return s;
}

void tst_QDebug::debugSpaceHandling() const
{
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        QVERIFY(d.autoInsertSpaces());
        d.setAutoInsertSpaces(false);
        QVERIFY(!d.autoInsertSpaces());
        d << "  ";
        d.setAutoInsertSpaces(true);
        QVERIFY(d.autoInsertSpaces());
        d << "foo";
        d.nospace();
        d << "key=" << "value";
        d.space();
        d << 1 << 2;
        MyLine line(MyPoint(10, 11), MyPoint (12, 13));
        d << line;
        d << "bar";
        // With the old implementation of MyPoint doing dbg.nospace() << ...; dbg.space() we ended up with
        // MyLine(MyPoint(10, 11) ,  MyPoint(12, 13) )
    }
    QCOMPARE(s_msg, QString::fromLatin1("  foo key=value 1 2 MyLine(MyPoint(10, 11), MyPoint(12, 13)) bar"));

    QVERIFY(qDebug().autoInsertSpaces());
    qDebug() << QPoint(21, 22) << QRect(23, 24, 25, 26) << QLine(27, 28, 29, 30);
    QCOMPARE(s_msg, QString::fromLatin1("QPoint(21,22) QRect(23,24 25x26) QLine(QPoint(27,28),QPoint(29,30))"));
    qDebug() << QPointF(21, 22) << QRectF(23, 24, 25, 26) << QLineF(27, 28, 29, 30);
    QCOMPARE(s_msg, QString::fromLatin1("QPointF(21,22) QRectF(23,24 25x26) QLineF(QPointF(27,28),QPointF(29,30))"));
    qDebug() << QMimeType() << QMimeDatabase().mimeTypeForName("application/pdf") << "foo";
    QCOMPARE(s_msg, QString::fromLatin1("QMimeType(invalid) QMimeType(\"application/pdf\") foo"));
}

void tst_QDebug::debugNoQuotes() const
{
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << QStringLiteral("Hello");
        d.noquote();
        d << QStringLiteral("Hello");
        d.quote();
        d << QStringLiteral("Hello");
    }
    QCOMPARE(s_msg, QString::fromLatin1("\"Hello\" Hello \"Hello\""));

    {
        QDebug d = qDebug();
        d << QChar('H');
        d << QLatin1String("Hello");
        d << QByteArray("Hello");
        d.noquote();
        d << QChar('H');
        d << QLatin1String("Hello");
        d << QByteArray("Hello");
    }
    QCOMPARE(s_msg, QString::fromLatin1("'H' \"Hello\" \"Hello\" H Hello Hello"));
}

void tst_QDebug::verbosity() const
{
    MyLine line(MyPoint(10, 11), MyPoint (12, 13));
    QString output;
    QDebug d(&output);
    d.nospace();
    d << line << '\n';
    const int oldVerbosity = d.verbosity();
    d.setVerbosity(0);
    QCOMPARE(d.verbosity(), 0);
    d.setVerbosity(7);
    QCOMPARE(d.verbosity(), 7);
    const int newVerbosity = oldVerbosity  + 2;
    d.setVerbosity(newVerbosity);
    QCOMPARE(d.verbosity(), newVerbosity);
    d << line << '\n';
    d.setVerbosity(oldVerbosity );
    QCOMPARE(d.verbosity(), oldVerbosity );
    d << line;
    const QStringList lines = output.split(QLatin1Char('\n'));
    QCOMPARE(lines.size(), 3);
    // Verbose should be longer
    QVERIFY2(lines.at(1).size() > lines.at(0).size(), qPrintable(lines.join(QLatin1Char(','))));
    // Switching back to brief produces same output
    QCOMPARE(lines.at(0).size(), lines.at(2).size());
}

void tst_QDebug::stateSaver() const
{
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << 42;
        {
            QDebugStateSaver saver(d);
            d << 43;
        }
        d << 44;
    }
    QCOMPARE(s_msg, QString::fromLatin1("42 43 44"));

    {
        QDebug d = qDebug();
        {
            QDebugStateSaver saver(d);
            d.nospace() << Qt::hex << Qt::right << qSetFieldWidth(3) << qSetPadChar('0') << 42;
        }
        d << 42;
    }
    QCOMPARE(s_msg, QString::fromLatin1("02a 42"));

    {
        QDebug d = qDebug();
        {
            QDebugStateSaver saver(d);
            d.nospace().noquote() << QStringLiteral("Hello");
        }
        d << QStringLiteral("World");
    }
    QCOMPARE(s_msg, QString::fromLatin1("Hello \"World\""));

    {
        QDebug d = qDebug();
        d.noquote().nospace() << QStringLiteral("Hello") << Qt::hex << 42;
        {
            QDebugStateSaver saver(d);
            d.resetFormat();
            d << QStringLiteral("World") << 42;
        }
        d << QStringLiteral("!") << 42;
    }
    QCOMPARE(s_msg, QString::fromLatin1("Hello2a\"World\" 42!2a"));
}

void tst_QDebug::veryLongWarningMessage() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    QString test;
    {
        QString part("0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\n");
        for (int i = 0; i < 1000; ++i)
            test.append(part);
        qWarning("Test output:\n%s\nend", qPrintable(test));
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 3; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtWarningMsg);
    QCOMPARE(s_msg, QString::fromLatin1("Test output:\n")+test+QString::fromLatin1("\nend"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);
}

void tst_QDebug::qDebugQChar() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << QChar('f') << QChar(QLatin1Char('\xE4')); // f, ä
        d.nospace().noquote() << QChar('o') << QChar('o')  << QChar(QLatin1Char('\xC4')); // o, o, Ä
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("'f' '\\u00e4' oo\\u00c4"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

}

void tst_QDebug::qDebugQMetaType() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << QMetaType::fromType<int>() << QMetaType::fromType<QString>();
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 4; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, R"(QMetaType(int) QMetaType(QString))"_L1);
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);
}

void tst_QDebug::qDebugQString() const
{
    /* Use a basic string. */
    {
        QString file, function;
        int line = 0;
        const QString in(QLatin1String("input"));
        const QStringView inRef{ in };

        MessageHandlerSetter mhs(myMessageHandler);
        { qDebug() << inRef; }
#ifndef QT_NO_MESSAGELOGCONTEXT
        file = __FILE__; line = __LINE__ - 2; function = Q_FUNC_INFO;
#endif
        QCOMPARE(s_msgType, QtDebugMsg);
        QCOMPARE(s_msg, QString::fromLatin1("\"input\""));
        QCOMPARE(QString::fromLatin1(s_file), file);
        QCOMPARE(s_line, line);
        QCOMPARE(QString::fromLatin1(s_function), function);
    }

    /* simpler tests from now on */
    MessageHandlerSetter mhs(myMessageHandler);

    QString string = "Hello";
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"Hello\""));

    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, string);

    qDebug().noquote().nospace() << qSetFieldWidth(8) << string;
    QCOMPARE(s_msg, "   " + string);

    string = "Sm\xc3\xb8rg\xc3\xa5sbord "                               // Latin script
             "\xce\x91\xce\xb8\xce\xae\xce\xbd\xce\xb1 "                // Greek script
             "\xd0\x9c\xd0\xbe\xd1\x81\xd0\xba\xd0\xb2\xd0\xb0";        // Cyrillic script
    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, string);

    // This string only contains printable characters
    qDebug() << string;
    QCOMPARE(s_msg, '"' + string + '"');

    string = "\n\t\\\"";
    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, string);

    // This string only contains characters that must be escaped
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\n\\t\\\\\\\"\""));

    // Unicode escapes, BMP
    string = "\1"                           // U+0001: START OF HEADING (category Cc)
             "\x7f"                         // U+007F: DELETE (category Cc)
             "\xc2\xad"                     // U+00AD: SOFT HYPHEN (category Cf)
             "\xef\xbb\xbf";                // U+FEFF: ZERO WIDTH NO-BREAK SPACE / BOM (category Cf)
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\u0001\\u007F\\u00AD\\uFEFF\""));

    // Unicode printable non-BMP
    string = "\xf0\x90\x80\x80";            // U+10000: LINEAR B SYLLABLE B008 A (category Lo)
    qDebug() << string;
    QCOMPARE(s_msg, '"' + string + '"');

    // non-BMP and non-printable
    string = "\xf3\xa0\x80\x81 "            // U+E0001: LANGUAGE TAG (category Cf)
             "\xf4\x80\x80\x80";            // U+100000: Plane 16 Private Use (category Co)
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\U000E0001 \\U00100000\""));

    // broken surrogate pairs
    char16_t utf16[] = { 0xDC00, 0xD800, 'x', 0xD800, 0 };
    string = QString::fromUtf16(utf16);
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\uDC00\\uD800x\\uD800\""));
}

void tst_QDebug::qDebugQStringView() const
{
    /* Use a basic string. */
    {
        QLatin1String file, function;
        int line = 0;
        const QStringView inView = u"input";

        MessageHandlerSetter mhs(myMessageHandler);
        { qDebug() << inView; }
#ifndef QT_NO_MESSAGELOGCONTEXT
        file = QLatin1String(__FILE__); line = __LINE__ - 2; function = QLatin1String(Q_FUNC_INFO);
#endif
        QCOMPARE(s_msgType, QtDebugMsg);
        QCOMPARE(s_msg, QLatin1String("\"input\""));
        QCOMPARE(QLatin1String(s_file), file);
        QCOMPARE(s_line, line);
        QCOMPARE(QLatin1String(s_function), function);
    }

    /* Use a null QStringView. */
    {
        QString file, function;
        int line = 0;

        const QStringView inView;

        MessageHandlerSetter mhs(myMessageHandler);
        { qDebug() << inView; }
#ifndef QT_NO_MESSAGELOGCONTEXT
        file = __FILE__; line = __LINE__ - 2; function = Q_FUNC_INFO;
#endif
        QCOMPARE(s_msgType, QtDebugMsg);
        QCOMPARE(s_msg, QLatin1String("\"\""));
        QCOMPARE(QLatin1String(s_file), file);
        QCOMPARE(s_line, line);
        QCOMPARE(QLatin1String(s_function), function);
    }
}

void tst_QDebug::qDebugQUtf8StringView() const
{
    /* Use a utf8 string. */
    {
        QLatin1String file, function;
        int line = 0;
        const QUtf8StringView inView = u8"\U0001F609 is ;-)";

        MessageHandlerSetter mhs(myMessageHandler);
        { qDebug() << inView; }
#ifndef QT_NO_MESSAGELOGCONTEXT
        file = QLatin1String(__FILE__); line = __LINE__ - 2; function = QLatin1String(Q_FUNC_INFO);
#endif
        QCOMPARE(s_msgType, QtDebugMsg);
        QCOMPARE(s_msg, QString::fromUtf8("\"\\xF0\\x9F\\x98\\x89 is ;-)\""));
        QCOMPARE(QLatin1String(s_file), file);
        QCOMPARE(s_line, line);
        QCOMPARE(QLatin1String(s_function), function);
    }

    /* Use a null QUtf8StringView. */
    {
        QString file, function;
        int line = 0;

        const QUtf8StringView inView;

        MessageHandlerSetter mhs(myMessageHandler);
        { qDebug() << inView; }
#ifndef QT_NO_MESSAGELOGCONTEXT
        file = __FILE__; line = __LINE__ - 2; function = Q_FUNC_INFO;
#endif
        QCOMPARE(s_msgType, QtDebugMsg);
        QCOMPARE(s_msg, QLatin1String("\"\""));
        QCOMPARE(QLatin1String(s_file), file);
        QCOMPARE(s_line, line);
        QCOMPARE(QLatin1String(s_function), function);
    }
}

void tst_QDebug::qDebugQLatin1String() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << QLatin1String("foo") << QLatin1String("") << QLatin1String("barbaz", 3);
        d.nospace().noquote() << QLatin1String("baz");
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("\"foo\" \"\" \"bar\" baz"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

    /* simpler tests from now on */
    QLatin1String string("\"Hello\"");
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\\"Hello\\\"\""));

    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, QString(string));

    qDebug().noquote().nospace() << qSetFieldWidth(8) << string;
    QCOMPARE(s_msg, " " + QString(string));

    string = QLatin1String("\nSm\xF8rg\xE5sbord\\");
    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, QString(string));

    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\nSm\\u00F8rg\\u00E5sbord\\\\\""));
}

void tst_QDebug::qDebugStdString() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << pmr::string("foo") << std::string("") << std::string("barbaz", 3);
        d.nospace().noquote() << std::string("baz");
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("\"foo\" \"\" \"bar\" baz"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

    /* simpler tests from now on */
    std::string string("\"Hello\"");
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\\"Hello\\\"\""));

    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, QString::fromStdString(string));

    qDebug().noquote().nospace() << qSetFieldWidth(8) << string;
    QCOMPARE(s_msg, " " + QString::fromStdString(string));
}

void tst_QDebug::qDebugStdStringView() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << std::string_view("foo") << std::string_view("") << std::string_view("barbaz", 3);
        d.nospace().noquote() << std::string_view("baz");
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("\"foo\" \"\" \"bar\" baz"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

    /* simpler tests from now on */
    std::string_view string("\"Hello\"");
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\\"Hello\\\"\""));

    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, QString::fromStdString(std::string(string)));

    qDebug().noquote().nospace() << qSetFieldWidth(8) << string;
    QCOMPARE(s_msg, " " + QString::fromStdString(std::string(string)));
}

void tst_QDebug::qDebugStdWString() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << pmr::wstring(L"foo") << std::wstring(L"") << std::wstring(L"barbaz", 3);
        d.nospace().noquote() << std::wstring(L"baz");
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("\"foo\" \"\" \"bar\" baz"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

    /* simpler tests from now on */
    std::wstring string(L"\"Hello\"");
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\\"Hello\\\"\""));

    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, QString::fromStdWString(string));

    qDebug().noquote().nospace() << qSetFieldWidth(8) << string;
    QCOMPARE(s_msg, " " + QString::fromStdWString(string));
}

void tst_QDebug::qDebugStdWStringView() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << std::wstring_view(L"foo") << std::wstring_view(L"") << std::wstring_view(L"barbaz", 3);
        d.nospace().noquote() << std::wstring_view(L"baz");
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("\"foo\" \"\" \"bar\" baz"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

    /* simpler tests from now on */
    std::wstring_view string(L"\"Hello\"");
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\\"Hello\\\"\""));

    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, QString::fromStdWString(std::wstring(string)));

    qDebug().noquote().nospace() << qSetFieldWidth(8) << string;
    QCOMPARE(s_msg, " " + QString::fromStdWString(std::wstring(string)));
}

void tst_QDebug::qDebugStdU8String() const
{
#ifndef __cpp_lib_char8_t
    QSKIP("This test requires C++20 char8_t support enabled in the compiler.");
#else
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << pmr::u8string(u8"foo") << std::u8string(u8"") << std::u8string(u8"barbaz", 3);
        d.nospace().noquote() << std::u8string(u8"baz");
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("\"foo\" \"\" \"bar\" baz"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

    /* simpler tests from now on */
    std::u8string string(u8"\"Hello\"");
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\\"Hello\\\"\""));

    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, QUtf8StringView(string).toString());

    qDebug().noquote().nospace() << qSetFieldWidth(8) << string;
    QCOMPARE(s_msg, " " + QUtf8StringView(string).toString());
#endif // __cpp_lib_char8_t
}

void tst_QDebug::qDebugStdU8StringView() const
{
#ifndef __cpp_lib_char8_t
    QSKIP("This test requires C++20 char8_t support enabled in the compiler.");
#else
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << std::u16string_view(u"foo") << std::u16string_view(u"") << std::u16string_view(u"barbaz", 3);
        d.nospace().noquote() << std::u16string_view(u"baz");
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("\"foo\" \"\" \"bar\" baz"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

    /* simpler tests from now on */
    std::u16string_view string(u"\"Hello\"");
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\\"Hello\\\"\""));

    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, QString::fromStdU16String(std::u16string(string)));

    qDebug().noquote().nospace() << qSetFieldWidth(8) << string;
    QCOMPARE(s_msg, " " + QString::fromStdU16String(std::u16string(string)));
#endif // __cpp_lib_char8_t
}

void tst_QDebug::qDebugStdU16String() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << pmr::u16string(u"foo") << std::u16string(u"") << std::u16string(u"barbaz", 3);
        d.nospace().noquote() << std::u16string(u"baz");
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("\"foo\" \"\" \"bar\" baz"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

    /* simpler tests from now on */
    std::u16string string(u"\"Hello\"");
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\\"Hello\\\"\""));

    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, QString::fromStdU16String(string));

    qDebug().noquote().nospace() << qSetFieldWidth(8) << string;
    QCOMPARE(s_msg, " " + QString::fromStdU16String(string));
}

void tst_QDebug::qDebugStdU16StringView() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << std::u16string_view(u"foo") << std::u16string_view(u"") << std::u16string_view(u"barbaz", 3);
        d.nospace().noquote() << std::u16string_view(u"baz");
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("\"foo\" \"\" \"bar\" baz"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

    /* simpler tests from now on */
    std::u16string_view string(u"\"Hello\"");
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\\"Hello\\\"\""));

    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, QString::fromStdU16String(std::u16string(string)));

    qDebug().noquote().nospace() << qSetFieldWidth(8) << string;
    QCOMPARE(s_msg, " " + QString::fromStdU16String(std::u16string(string)));
}

void tst_QDebug::qDebugStdU32String() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << pmr::u32string(U"foo") << std::u32string(U"") << std::u32string(U"barbaz", 3);
        d.nospace().noquote() << std::u32string(U"baz");
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("\"foo\" \"\" \"bar\" baz"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

    /* simpler tests from now on */
    std::u32string string(U"\"Hello\"");
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\\"Hello\\\"\""));

    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, QString::fromStdU32String(string));

    qDebug().noquote().nospace() << qSetFieldWidth(8) << string;
    QCOMPARE(s_msg, " " + QString::fromStdU32String(string));
}

void tst_QDebug::qDebugStdU32StringView() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << std::u32string_view(U"foo") << std::u32string_view(U"") << std::u32string_view(U"barbaz", 3);
        d.nospace().noquote() << std::u32string_view(U"baz");
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("\"foo\" \"\" \"bar\" baz"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

    /* simpler tests from now on */
    std::u32string_view string(U"\"Hello\"");
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\\"Hello\\\"\""));

    qDebug().noquote().nospace() << string;
    QCOMPARE(s_msg, QString::fromStdU32String(std::u32string(string)));

    qDebug().noquote().nospace() << qSetFieldWidth(8) << string;
    QCOMPARE(s_msg, " " + QString::fromStdU32String(std::u32string(string)));
}

void tst_QDebug::qDebugQByteArray() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << QByteArrayLiteral("foo") << QByteArrayLiteral("") << QByteArray("barbaz", 3);
        d.nospace().noquote() << QByteArrayLiteral("baz");
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("\"foo\" \"\" \"bar\" baz"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

    /* simpler tests from now on */
    QByteArray ba = "\"Hello\"";
    qDebug() << ba;
    QCOMPARE(s_msg, QString("\"\\\"Hello\\\"\""));

    qDebug().noquote().nospace() << ba;
    QCOMPARE(s_msg, QString::fromLatin1(ba));

    qDebug().noquote().nospace() << qSetFieldWidth(8) << ba;
    QCOMPARE(s_msg, " " + QString::fromLatin1(ba));

    ba = "\nSm\xC3\xB8rg\xC3\xA5sbord\\";
    qDebug().noquote().nospace() << ba;
    QCOMPARE(s_msg, QString::fromUtf8(ba));

    qDebug() << ba;
    QCOMPARE(s_msg, QString("\"\\nSm\\xC3\\xB8rg\\xC3\\xA5sbord\\\\\""));

    // ensure that it closes hex escape sequences correctly
    qDebug() << QByteArray("\377FFFF");
    QCOMPARE(s_msg, QString("\"\\xFF\"\"FFFF\""));
}

void tst_QDebug::qDebugQByteArrayView() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d << QByteArrayView("foo") << QByteArrayView("") << QByteArrayView("barbaz", 3);
        d.nospace().noquote() << QByteArrayView("baz");
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("\"foo\" \"\" \"bar\" baz"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

    /* simpler tests from now on */
    QByteArrayView ba = "\"Hello\"";
    qDebug() << ba;
    QCOMPARE(s_msg, QString("\"\\\"Hello\\\"\""));

    qDebug().noquote().nospace() << ba;
    QCOMPARE(s_msg, QString::fromLatin1(ba));

    qDebug().noquote().nospace() << qSetFieldWidth(8) << ba;
    QCOMPARE(s_msg, " " + QString::fromLatin1(ba));

    ba = "\nSm\xC3\xB8rg\xC3\xA5sbord\\";
    qDebug().noquote().nospace() << ba;
    QCOMPARE(s_msg, QString::fromUtf8(ba));

    qDebug() << ba;
    QCOMPARE(s_msg, QString("\"\\nSm\\xC3\\xB8rg\\xC3\\xA5sbord\\\\\""));

    // ensure that it closes hex escape sequences correctly
    qDebug() << QByteArrayView("\377FFFF");
    QCOMPARE(s_msg, QString("\"\\xFF\"\"FFFF\""));
}

enum TestEnum {
    Flag1 = 0x1,
    Flag2 = 0x10
};

Q_DECLARE_FLAGS(TestFlags, TestEnum)

void tst_QDebug::qDebugQFlags() const
{
    QString file, function;
    int line = 0;
    QFlags<TestEnum> flags(Flag1 | Flag2);

    MessageHandlerSetter mhs(myMessageHandler);
    { qDebug() << flags; }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 2; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("QFlags(0x1|0x10)"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);

    // Test the output of QFlags with an enum not declared with Q_DECLARE_FLAGS and Q_FLAGS
    QFlags<EnumType> flags2(EnumValue2);
    qDebug() << flags2;
    QCOMPARE(s_msg, QString::fromLatin1("QFlags<tst_QDebug::EnumType>(EnumValue2)"));

    // A now for one that was fully declared
    tst_QDebug::Flags flags3(EnumFlag1);
    qDebug() << flags3;
    QCOMPARE(s_msg, QString::fromLatin1("QFlags<tst_QDebug::FlagType>(EnumFlag1)"));
}

using ToStringFunction = std::function<QString()>;
void tst_QDebug::qDebugStdChrono_data() const
{
    using attoseconds = duration<int64_t, std::atto>;
    using femtoseconds = duration<int64_t, std::femto>;
    using picoseconds = duration<int64_t, std::pico>;
    using centiseconds = duration<int64_t, std::centi>;
    using deciseconds = duration<int64_t, std::deci>;

    using quadriennia = duration<int, std::ratio_multiply<std::ratio<4>, years::period>>;
    using decades = duration<int, std::ratio_multiply<years::period, std::deca>>; // decayears
    using centuries = duration<int16_t, std::ratio_multiply<years::period, std::hecto>>; // hectoyears
    using millennia = duration<int16_t, std::ratio_multiply<years::period, std::kilo>>; // kiloyears
    using gigayears = duration<int8_t, std::ratio_multiply<years::period, std::giga>>;
    using fortnights = duration<int, std::ratio_multiply<days::period, std::ratio<14>>>;
    using microfortnights = duration<int64_t, std::ratio_multiply<fortnights::period, std::micro>>;
    using telecom = duration<int64_t, std::ratio<1, 8000>>; // 8 kHz

    using kiloseconds = duration<int64_t, std::kilo>;
    using exaseconds = duration<int8_t, std::exa>;
    using meter_per_light = duration<int64_t, std::ratio<1, 299'792'458>>;
    using kilometer_per_light = duration<int64_t, std::ratio<1000, 299'792'458>>;

    QTest::addColumn<ToStringFunction>("fn");
    QTest::addColumn<QString>("expected");

    auto addRow = [](const char *name, auto duration, const char *expected) {
        auto toString = [duration]() { return QDebug::toString(duration); };
        QTest::newRow(name) << ToStringFunction(toString) << expected;
    };

    addRow("1as", attoseconds{1}, "1as");
    addRow("1fs", femtoseconds{1}, "1fs");
    addRow("1ps", picoseconds{1}, "1ps");
    addRow("0ns", 0ns, "0ns");
    addRow("1000ns", 1000ns, "1000ns");
    addRow("0us", 0us, "0us");
    addRow("0ms", 0ms, "0ms");
    addRow("1cs", centiseconds{1}, "1cs");
    addRow("2ds", deciseconds{2}, "2ds");
    addRow("-1s", -1s, "-1s");
    addRow("0s", 0s, "0s");
    addRow("1s", 1s, "1s");
    addRow("60s", 60s, "60s");
    addRow("1min", 1min, "1min");
    addRow("1h", 1h, "1h");
    addRow("1days", days{1}, "1d");
    addRow("365days", days{365}, "365d");
    addRow("1weeks", weeks{1}, "1wk");
    addRow("1years", years{1}, "1yr"); // 365.2425 days
    addRow("42years", years{42}, "42yr");

    addRow("1ks", kiloseconds{1}, "1[1000]s");
    addRow("2fortnights", fortnights{2}, "2[2]wk");
    addRow("1quadriennia", quadriennia{1}, "1[4]yr");
    addRow("1decades", decades{1}, "1[10]yr");
    addRow("1centuries", centuries{1}, "1[100]yr");
    addRow("1millennia", millennia{1}, "1[1000]yr");
#if defined(Q_OS_LINUX) || defined(Q_OS_DARWIN)
    // some OSes print the exponent differently
    addRow("1Es", exaseconds{1}, "1[1e+18]s");
    addRow("13gigayears", gigayears{13}, "13[1e+09]yr");
#endif

    // months are one twelfth of a Gregorian year, not 30 days
    addRow("1months", months{1}, "1[2629746]s");

    // weird units
    addRow("2microfortnights", microfortnights{2}, "2[756/625]s");
    addRow("1telecom", telecom{1}, "1[1/8000]s");
    addRow("10m/c", meter_per_light{10}, "10[1/299792458]s");
    addRow("10km/c", kilometer_per_light{10}, "10[500/149896229]s");

    // real floting point
    using fpsec = duration<double>;
    using fpmsec = duration<double, std::milli>;
    using fpnsec = duration<double, std::nano>;
    addRow("1.0s", fpsec{1}, "1s");
    addRow("1.5s", fpsec{1.5}, "1.5s");
    addRow("1.0ms", fpmsec{1}, "1ms");
    addRow("1.5ms", fpmsec{1.5}, "1.5ms");
    addRow("1.0ns", fpnsec{1}, "1ns");
    addRow("1.5ns", fpnsec{1.5}, "1.5ns");

    // and some precision setting too
    QTest::newRow("1.00000ns")
            << ToStringFunction([]() {
        QString buffer;
        QDebug d(&buffer);
        d.nospace() << qSetRealNumberPrecision(5) << Qt::fixed << fpnsec{1};
        return buffer;
    }) << "1.00000ns";
}

void tst_QDebug::qDebugStdChrono() const
{
    QFETCH(ToStringFunction, fn);
    QFETCH(QString, expected);
    QCOMPARE(fn(), expected);
}

void tst_QDebug::textStreamModifiers() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    { qDebug() << Qt::hex << short(0xf) << int(0xf) << unsigned(0xf) << long(0xf) << qint64(0xf) << quint64(0xf); }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 2; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("f f f f f f"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);
}

void tst_QDebug::resetFormat() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        d.nospace().noquote() << Qt::hex <<  int(0xf);
        d.resetFormat() << int(0xf) << QStringLiteral("foo");
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("f15 \"foo\""));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);
}

void tst_QDebug::defaultMessagehandler() const
{
    MessageHandlerSetter mhs(0); // set 0, should set default handler
    QtMessageHandler defaultMessageHandler1 = qInstallMessageHandler((QtMessageHandler)0); // set 0, should set and return default handler
    QVERIFY(defaultMessageHandler1);
    QtMessageHandler defaultMessageHandler2 = qInstallMessageHandler(myMessageHandler); // set myMessageHandler and return default handler
    bool same = (*defaultMessageHandler1 == *defaultMessageHandler2);
    QVERIFY(same);
    QtMessageHandler messageHandler = qInstallMessageHandler((QtMessageHandler)0); // set 0, should set default and return myMessageHandler
    same = (*messageHandler == *myMessageHandler);
    QVERIFY(same);
}

QMutex s_mutex;
QStringList s_messages;
QSemaphore s_sema;

static void threadSafeMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QMutexLocker lock(&s_mutex);
    s_messages.append(msg);
    Q_UNUSED(type);
    Q_UNUSED(context);
}

static void doDebug() // called in each thread
{
    s_sema.acquire();
    qDebug() << "doDebug";
}

void tst_QDebug::threadSafety() const
{
    MessageHandlerSetter mhs(threadSafeMessageHandler);
    const int numThreads = 10;
    QThreadPool::globalInstance()->setMaxThreadCount(numThreads);
    QFutureSynchronizer<void> sync;
    for (int i = 0; i < numThreads; ++i) {
        sync.addFuture(QtConcurrent::run(&doDebug));
    }
    s_sema.release(numThreads);
    sync.waitForFinished();
    QMutexLocker lock(&s_mutex);
    QCOMPARE(s_messages.size(), numThreads);
    for (int i = 0; i < numThreads; ++i) {
        QCOMPARE(s_messages.at(i), QStringLiteral("doDebug"));
    }
}

void tst_QDebug::toString() const
{
    // By reference.
    {
        MyPoint point(3, 4);
        QString expectedString;
        QDebug stream(&expectedString);
        stream.nospace() << point;
        QCOMPARE(QDebug::toString(point), expectedString);
    }

    // By pointer.
    {
        QObject qobject;
        qobject.setObjectName("test");
        QString expectedString;
        QDebug stream(&expectedString);
        stream.nospace() << &qobject;
        QCOMPARE(QDebug::toString(&qobject), expectedString);
    }
}

void tst_QDebug::noQVariantEndlessRecursion() const
{
    ConvertsToQVariant conv;
    QVariant var = QVariant::fromValue(conv);
    QTest::ignoreMessage(QtDebugMsg, "QVariant(ConvertsToQVariant, )");
    qDebug() << var;
}

#if defined(Q_OS_DARWIN)

@interface MyObjcClass : NSObject
@end

@implementation MyObjcClass : NSObject
- (NSString *)description
{
    return @"MyObjcClass is the best";
}
@end

void tst_QDebug::objcInCppMode_data() const
{
    QTest::addColumn<objc_object *>("object");
    QTest::addColumn<QString>("message");

    QTest::newRow("nil") << static_cast<objc_object*>(nullptr) << QString::fromLatin1("(null)");

    // Not an NSObject subclass
    auto *nsproxy = reinterpret_cast<objc_object *>(class_createInstance(objc_getClass("NSProxy"), 0));
    QTest::newRow("NSProxy") << nsproxy << QString::fromLatin1("<NSProxy: 0x%1>").arg(uintptr_t(nsproxy), 1, 16);

    // Plain NSObject
    auto *nsobject = reinterpret_cast<objc_object *>(class_createInstance(objc_getClass("NSObject"), 0));
    QTest::newRow("NSObject") << nsobject << QString::fromLatin1("<NSObject: 0x%1>").arg(uintptr_t(nsobject), 1, 16);

    auto str = QString::fromLatin1("foo");
    QTest::newRow("NSString") << reinterpret_cast<objc_object*>(str.toNSString()) << str;

    // Custom debug description
    QTest::newRow("MyObjcClass") << reinterpret_cast<objc_object*>([[MyObjcClass alloc] init])
                                 << QString::fromLatin1("MyObjcClass is the best");
}

void tst_QDebug::objcInCppMode() const
{
    QFETCH(objc_object *, object);
    QFETCH(QString, message);

    MessageHandlerSetter mhs(myMessageHandler);
    { qDebug() << object; }

    QCOMPARE(s_msg, message);
}

void tst_QDebug::objcInObjcMode_data() const
{
    objcInCppMode_data();
}

void tst_QDebug::objcInObjcMode() const
{
    QFETCH(objc_object *, object);
    QFETCH(QString, message);

    MessageHandlerSetter mhs(myMessageHandler);
    { qDebug() << static_cast<id>(object); }

    QCOMPARE(s_msg, message);
}
#endif

// Should compile: instentiation of unrelated operator<< should not cause cause compilation
// error in QDebug operators (QTBUG-47375)
class TestClassA {};
class TestClassB {};

template <typename T>
TestClassA& operator<< (TestClassA& s, T&) { return s; };
template<> TestClassA& operator<< <TestClassB>(TestClassA& s, TestClassB& l);

QTEST_MAIN(tst_QDebug);
#include "tst_qdebug.moc"
