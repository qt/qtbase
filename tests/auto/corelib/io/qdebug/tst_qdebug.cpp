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


#include <QtCore/QCoreApplication>
#include <QtCore/QtDebug>
#include <QtTest/QtTest>

class tst_QDebug: public QObject
{
    Q_OBJECT
private slots:
    void assignment() const;
    void warningWithoutDebug() const;
    void criticalWithoutDebug() const;
    void debugWithBool() const;
    void debugSpaceHandling() const;
    void stateSaver() const;
    void veryLongWarningMessage() const;
    void qDebugQStringRef() const;
    void qDebugQLatin1String() const;
    void defaultMessagehandler() const;
};

void tst_QDebug::assignment() const
{
    QDebug debug1(QtDebugMsg);
    QDebug debug2(QtWarningMsg);

    QTest::ignoreMessage(QtDebugMsg, "foo ");
    QTest::ignoreMessage(QtWarningMsg, "bar 1 2 ");

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
    MessageHandlerSetter mhs(myMessageHandler);
    { qWarning() << "A qWarning() message"; }
    QString file = __FILE__; int line = __LINE__ - 1; QString function = Q_FUNC_INFO;
    QCOMPARE(s_msgType, QtWarningMsg);
    QCOMPARE(s_msg, QString::fromLatin1("A qWarning() message "));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);
}

/*! \internal
  The qCritical() stream should be usable even if QT_NO_DEBUG is defined.
 */
void tst_QDebug::criticalWithoutDebug() const
{
    MessageHandlerSetter mhs(myMessageHandler);
    { qCritical() << "A qCritical() message"; }
    QString file = __FILE__; int line = __LINE__ - 1; QString function = Q_FUNC_INFO;
    QCOMPARE(s_msgType, QtCriticalMsg);
    QCOMPARE(s_msg, QString::fromLatin1("A qCritical() message "));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);
}

void tst_QDebug::debugWithBool() const
{
    MessageHandlerSetter mhs(myMessageHandler);
    { qDebug() << false << true; }
    QString file = __FILE__; int line = __LINE__ - 1; QString function = Q_FUNC_INFO;
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("false true "));
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
    return s.nospace() << "MyPoint(" << point.v1 << ", " << point.v2 << ")";
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
    s.nospace() << "MyLine(" << line.p1 << ", " << line.p2 << ")";
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
        // With the old implementation of MyPoint doing dbg.nospace() << ...; dbg.space() we ended up with
        // MyLine(MyPoint(10, 11) ,  MyPoint(12, 13) )
    }
    QCOMPARE(s_msg, QString::fromLatin1("  foo key=value 1 2 MyLine(MyPoint(10, 11), MyPoint(12, 13))"));
}

void tst_QDebug::stateSaver() const
{
    MessageHandlerSetter mhs(myMessageHandler);
    {
        QDebug d = qDebug();
        {
            QDebugStateSaver saver(d);
            d.nospace() << hex << right << qSetFieldWidth(3) << qSetPadChar('0') << 42;
        }
        d.space() << 42;
    }
    QCOMPARE(s_msg, QString::fromLatin1("02a 42 "));
}

void tst_QDebug::veryLongWarningMessage() const
{
    MessageHandlerSetter mhs(myMessageHandler);
    QString test;
    {
        QString part("0123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789\n");
        for (int i = 0; i < 1000; ++i)
            test.append(part);
        qWarning("Test output:\n%s\nend", qPrintable(test));
    }
    QString file = __FILE__; int line = __LINE__ - 2; QString function = Q_FUNC_INFO;
    QCOMPARE(s_msgType, QtWarningMsg);
    QCOMPARE(s_msg, QString::fromLatin1("Test output:\n")+test+QString::fromLatin1("\nend"));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);
}

void tst_QDebug::qDebugQStringRef() const
{
    /* Use a basic string. */
    {
        const QString in(QLatin1String("input"));
        const QStringRef inRef(&in);

        MessageHandlerSetter mhs(myMessageHandler);
        { qDebug() << inRef; }
        QString file = __FILE__; int line = __LINE__ - 1; QString function = Q_FUNC_INFO;
        QCOMPARE(s_msgType, QtDebugMsg);
        QCOMPARE(s_msg, QString::fromLatin1("\"input\" "));
        QCOMPARE(QString::fromLatin1(s_file), file);
        QCOMPARE(s_line, line);
        QCOMPARE(QString::fromLatin1(s_function), function);
    }

    /* Use a null QStringRef. */
    {
        const QStringRef inRef;

        MessageHandlerSetter mhs(myMessageHandler);
        { qDebug() << inRef; }
        QString file = __FILE__; int line = __LINE__ - 1; QString function = Q_FUNC_INFO;
        QCOMPARE(s_msgType, QtDebugMsg);
        QCOMPARE(s_msg, QString::fromLatin1("\"\" "));
        QCOMPARE(QString::fromLatin1(s_file), file);
        QCOMPARE(s_line, line);
        QCOMPARE(QString::fromLatin1(s_function), function);
    }
}

void tst_QDebug::qDebugQLatin1String() const
{
    MessageHandlerSetter mhs(myMessageHandler);
    { qDebug() << QLatin1String("foo") << QLatin1String("") << QLatin1String("barbaz", 3); }
    QString file = __FILE__; int line = __LINE__ - 1; QString function = Q_FUNC_INFO;
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(s_msg, QString::fromLatin1("\"foo\" \"\" \"bar\" "));
    QCOMPARE(QString::fromLatin1(s_file), file);
    QCOMPARE(s_line, line);
    QCOMPARE(QString::fromLatin1(s_function), function);
}

void tst_QDebug::defaultMessagehandler() const
{
    MessageHandlerSetter mhs(0);
    QtMessageHandler defaultMessageHandler1 = qInstallMessageHandler((QtMessageHandler)0);
    QtMessageHandler defaultMessageHandler2 = qInstallMessageHandler(myMessageHandler);
    bool same = (*defaultMessageHandler1 == *defaultMessageHandler2);
    QVERIFY(same);
    QtMessageHandler messageHandler = qInstallMessageHandler((QtMessageHandler)0);
    same = (*messageHandler == *myMessageHandler);
    QVERIFY(same);
}

QTEST_MAIN(tst_QDebug);
#include "tst_qdebug.moc"
