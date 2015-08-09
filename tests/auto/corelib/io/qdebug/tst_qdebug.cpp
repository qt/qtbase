/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/


#include <QtCore/QCoreApplication>
#include <QtCore/QtDebug>
#include <QtTest/QtTest>

#include <QtConcurrentRun>
#include <QFutureSynchronizer>

class tst_QDebug: public QObject
{
    Q_OBJECT
private slots:
    void assignment() const;
    void warningWithoutDebug() const;
    void criticalWithoutDebug() const;
    void debugWithBool() const;
    void debugSpaceHandling() const;
    void debugNoQuotes() const;
    void verbosity() const;
    void stateSaver() const;
    void veryLongWarningMessage() const;
    void qDebugQChar() const;
    void qDebugQString() const;
    void qDebugQStringRef() const;
    void qDebugQLatin1String() const;
    void qDebugQByteArray() const;
    void qDebugQFlags() const;
    void textStreamModifiers() const;
    void resetFormat() const;
    void defaultMessagehandler() const;
    void threadSafety() const;
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
            d.nospace() << hex << right << qSetFieldWidth(3) << qSetPadChar('0') << 42;
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
        d.noquote().nospace() << QStringLiteral("Hello") << hex << 42;
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
        d << QChar('f');
        d.nospace().noquote() << QChar('o') << QChar('o');
    }
#ifndef QT_NO_MESSAGELOGCONTEXT
    file = __FILE__; line = __LINE__ - 5; function = Q_FUNC_INFO;
#endif
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("'f' oo"));
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
        const QStringRef inRef(&in);

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
    ushort utf16[] = { 0xDC00, 0xD800, 'x', 0xD800, 0 };
    string = QString::fromUtf16(utf16);
    qDebug() << string;
    QCOMPARE(s_msg, QString("\"\\uDC00\\uD800x\\uD800\""));
}

void tst_QDebug::qDebugQStringRef() const
{
    /* Use a basic string. */
    {
        QString file, function;
        int line = 0;
        const QString in(QLatin1String("input"));
        const QStringRef inRef(&in);

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

    /* Use a null QStringRef. */
    {
        QString file, function;
        int line = 0;

        const QStringRef inRef;

        MessageHandlerSetter mhs(myMessageHandler);
        { qDebug() << inRef; }
#ifndef QT_NO_MESSAGELOGCONTEXT
        file = __FILE__; line = __LINE__ - 2; function = Q_FUNC_INFO;
#endif
        QCOMPARE(s_msgType, QtDebugMsg);
        QCOMPARE(s_msg, QString::fromLatin1("\"\""));
        QCOMPARE(QString::fromLatin1(s_file), file);
        QCOMPARE(s_line, line);
        QCOMPARE(QString::fromLatin1(s_function), function);
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

}

void tst_QDebug::textStreamModifiers() const
{
    QString file, function;
    int line = 0;
    MessageHandlerSetter mhs(myMessageHandler);
    { qDebug() << hex << short(0xf) << int(0xf) << unsigned(0xf) << long(0xf) << qint64(0xf) << quint64(0xf); }
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
        d.nospace().noquote() << hex <<  int(0xf);
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
    QCOMPARE(s_messages.count(), numThreads);
    for (int i = 0; i < numThreads; ++i) {
        QCOMPARE(s_messages.at(i), QStringLiteral("doDebug"));
    }
}

// Should compile: instentiation of unrelated operator<< should not cause cause compilation
// error in QDebug operators (QTBUG-47375)
class TestClassA {};
class TestClassB {};

template <typename T>
TestClassA& operator<< (TestClassA& s, T&) { return s; };
template<> TestClassA& operator<< <TestClassB>(TestClassA& s, TestClassB& l);

QTEST_MAIN(tst_QDebug);
#include "tst_qdebug.moc"
