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

#include <qobject.h>

class tst_QObjectPerformance : public QObject
{
    Q_OBJECT

public:

private slots:
    void emitToManyReceivers();
};

class SimpleSenderObject : public QObject
{
    Q_OBJECT

signals:
    void signal();

public:
    void emitSignal()
    {
        emit signal();
    }
};

class SimpleReceiverObject : public QObject
{
    Q_OBJECT

public slots:
    void slot()
    {
    }
};

void tst_QObjectPerformance::emitToManyReceivers()
{
    // ensure that emission times remain mostly linear as the number of receivers increase

    SimpleSenderObject sender;
    int elapsed = 0;
    const int increase = 3000;
    const int base = 5000;

    for (int i = 0; i < 4; ++i) {
        const int size = base + i * increase;
        const double increaseRatio = double(size) / (double)(size - increase);

        QList<SimpleReceiverObject *> receivers;
        for (int k = 0; k < size; ++k) {
            SimpleReceiverObject *receiver = new SimpleReceiverObject;
            QObject::connect(&sender, SIGNAL(signal()), receiver, SLOT(slot()));
            receivers.append(receiver);
        }

        QTime timer;
        timer.start();
        sender.emitSignal();
        int e = timer.elapsed();

        if (elapsed > 1) {
            qDebug() << size << "receivers, elapsed time" << e << "compared to previous time" << elapsed;
            QVERIFY(double(e) / double(elapsed) <= increaseRatio * 2.0);
        } else {
            qDebug() << size << "receivers, elapsed time" << e << "cannot be compared to previous, unmeasurable time";
        }
        elapsed = e;

        qDeleteAll(receivers);
        receivers.clear();
    }
}


QTEST_MAIN(tst_QObjectPerformance)
#include "tst_qobjectperformance.moc"
