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

#include <qdebug.h>
#include <QtTest/QtTest>

#include <qglobal.h>

class tst_qmessagehandler : public QObject
{
    Q_OBJECT
private slots:
    void cleanup();

    void defaultHandler();
    void installMessageHandler();
    void installMsgHandler();
    void installBothHandler();
};

static QtMsgType s_type;
const char *s_file;
int s_line;
const char *s_function;
static QString s_message;

void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const char *msg)
{
    s_type = type;
    s_file = context.file;
    s_line = context.line;
    s_function = context.function;
    s_message = QString::fromLocal8Bit(msg);
}

void customMsgHandler(QtMsgType type, const char *msg)
{
    s_type = type;
    s_file = 0;
    s_line = 0;
    s_function = 0;
    s_message = QString::fromLocal8Bit(msg);
}

void tst_qmessagehandler::cleanup()
{
    qInstallMsgHandler(0);
    qInstallMessageHandler(0);
    s_type = QtFatalMsg;
    s_file = 0;
    s_line = 0;
    s_function = 0;
}

void tst_qmessagehandler::defaultHandler()
{
    // check that the default works
    QTest::ignoreMessage(QtDebugMsg, "defaultHandler");
    qDebug("defaultHandler");
}

void tst_qmessagehandler::installMessageHandler()
{
    QMessageHandler oldHandler = qInstallMessageHandler(customMessageHandler);

    qDebug("installMessageHandler"); int line = __LINE__;

    QCOMPARE(s_type, QtDebugMsg);
    QCOMPARE(s_message, QString::fromLocal8Bit("installMessageHandler"));
    QCOMPARE(s_file, __FILE__);
    QCOMPARE(s_function, Q_FUNC_INFO);
    QCOMPARE(s_line, line);

    QMessageHandler myHandler = qInstallMessageHandler(oldHandler);
    QCOMPARE((void*)myHandler, (void*)customMessageHandler);
}

void tst_qmessagehandler::installMsgHandler()
{
    QtMsgHandler oldHandler = qInstallMsgHandler(customMsgHandler);

    qDebug("installMsgHandler");

    QCOMPARE(s_type, QtDebugMsg);
    QCOMPARE(s_message, QString::fromLocal8Bit("installMsgHandler"));
    QCOMPARE(s_file, (const char*)0);
    QCOMPARE(s_function, (const char*)0);
    QCOMPARE(s_line, 0);

    QtMsgHandler myHandler = qInstallMsgHandler(oldHandler);
    QCOMPARE((void*)myHandler, (void*)customMsgHandler);
}

void tst_qmessagehandler::installBothHandler()
{
    qInstallMessageHandler(customMessageHandler);
    qInstallMsgHandler(customMsgHandler);

    qDebug("installBothHandler"); int line = __LINE__;

    QCOMPARE(s_type, QtDebugMsg);
    QCOMPARE(s_message, QString::fromLocal8Bit("installBothHandler"));
    QCOMPARE(s_file, __FILE__);
    QCOMPARE(s_function, Q_FUNC_INFO);
    QCOMPARE(s_line, line);
}

QTEST_MAIN(tst_qmessagehandler)
#include "tst_qmessagehandler.moc"
