/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtCore>
#include <QtTest/QtTest>

#define INVOKE_COUNT 10000

class qtimer_vs_qmetaobject : public QObject
{
    Q_OBJECT
private slots:
    void testZeroTimerSingleShot();
    void testQueuedInvokeMethod();
};

class InvokeCounter : public QObject {
    Q_OBJECT
public:
    InvokeCounter() : count(0) { };
public slots:
    void invokeSlot() {
        count++;
        if (count == INVOKE_COUNT)
            QTestEventLoop::instance().exitLoop();
    }
protected:
    int count;
};

void qtimer_vs_qmetaobject::testZeroTimerSingleShot()
{
    QBENCHMARK {
        InvokeCounter invokeCounter;
        for(int i = 0; i < INVOKE_COUNT; ++i) {
            QTimer::singleShot(0, &invokeCounter, SLOT(invokeSlot()));
        }
        QTestEventLoop::instance().enterLoop(10);
        QVERIFY(!QTestEventLoop::instance().timeout());
    }
}

void qtimer_vs_qmetaobject::testQueuedInvokeMethod()
{
    QBENCHMARK {
        InvokeCounter invokeCounter;
        for(int i = 0; i < INVOKE_COUNT; ++i) {
            QMetaObject::invokeMethod(&invokeCounter, "invokeSlot", Qt::QueuedConnection);
        }
        QTestEventLoop::instance().enterLoop(10);
        QVERIFY(!QTestEventLoop::instance().timeout());
    }
}


QTEST_MAIN(qtimer_vs_qmetaobject)

#include "tst_qtimer_vs_qmetaobject.moc"
