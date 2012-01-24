/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
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
    void veryLongWarningMessage() const;
    void qDebugQStringRef() const;
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
static QByteArray s_msg;

static void myMessageHandler(QtMsgType type, const char *msg)
{
    s_msg = msg;
    s_msgType = type;
}

// Helper class to ensure that the testlib message handler gets
// restored at the end of each test function, even if the test
// fails or throws an exception.
class MessageHandlerSetter
{
public:
    MessageHandlerSetter(QtMsgHandler newMsgHandler)
        : oldMsgHandler(qInstallMsgHandler(newMsgHandler))
    { }

    ~MessageHandlerSetter()
    {
        qInstallMsgHandler(oldMsgHandler);
    }

private:
    QtMsgHandler oldMsgHandler;
};

/*! \internal
  The qWarning() stream should be usable even if QT_NO_DEBUG is defined.
 */
void tst_QDebug::warningWithoutDebug() const
{
    MessageHandlerSetter mhs(myMessageHandler);
    { qWarning() << "A qWarning() message"; }
    QCOMPARE(s_msgType, QtWarningMsg);
    QCOMPARE(QString::fromLatin1(s_msg.data()), QString::fromLatin1("A qWarning() message "));
}

/*! \internal
  The qCritical() stream should be usable even if QT_NO_DEBUG is defined.
 */
void tst_QDebug::criticalWithoutDebug() const
{
    MessageHandlerSetter mhs(myMessageHandler);
    { qCritical() << "A qCritical() message"; }
    QCOMPARE(s_msgType, QtCriticalMsg);
    QCOMPARE(QString::fromLatin1(s_msg), QString::fromLatin1("A qCritical() message "));
}

void tst_QDebug::debugWithBool() const
{
    MessageHandlerSetter mhs(myMessageHandler);
    { qDebug() << false << true; }
    QCOMPARE(s_msgType, QtDebugMsg);
    QCOMPARE(QString::fromLatin1(s_msg), QString::fromLatin1("false true "));
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
    QCOMPARE(s_msgType, QtWarningMsg);
    QCOMPARE(QString::fromLatin1(s_msg), QString::fromLatin1("Test output:\n")+test+QString::fromLatin1("\nend"));
}

void tst_QDebug::qDebugQStringRef() const
{
    /* Use a basic string. */
    {
        const QString in(QLatin1String("input"));
        const QStringRef inRef(&in);

        MessageHandlerSetter mhs(myMessageHandler);
        { qDebug() << inRef; }
        QCOMPARE(s_msgType, QtDebugMsg);
        QCOMPARE(QString::fromLatin1(s_msg), QString::fromLatin1("\"input\" "));
    }

    /* Use a null QStringRef. */
    {
        const QStringRef inRef;

        MessageHandlerSetter mhs(myMessageHandler);
        { qDebug() << inRef; }
        QCOMPARE(s_msgType, QtDebugMsg);
        QCOMPARE(QString::fromLatin1(s_msg), QString::fromLatin1("\"\" "));
    }
}

void tst_QDebug::defaultMessagehandler() const
{
    MessageHandlerSetter mhs(0);
    QtMsgHandler defaultMessageHandler1 = qInstallMsgHandler(0);
    QtMsgHandler defaultMessageHandler2 = qInstallMsgHandler(myMessageHandler);
    bool same = (*defaultMessageHandler1 == *defaultMessageHandler2);
    QVERIFY(same);
    QtMsgHandler messageHandler = qInstallMsgHandler(0);
    same = (*messageHandler == *myMessageHandler);
    QVERIFY(same);
}

QTEST_MAIN(tst_QDebug);
#include "tst_qdebug.moc"
