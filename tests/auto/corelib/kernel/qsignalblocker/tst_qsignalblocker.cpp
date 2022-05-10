// Copyright (C) 2013 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QTest>

#include "qobject.h"

class tst_QSignalBlocker : public QObject
{
    Q_OBJECT
private slots:
    void signalBlocking();
    void moveAssignment();
};

void tst_QSignalBlocker::signalBlocking()
{
    QObject o;

    QVERIFY(!o.signalsBlocked());

    {
        QSignalBlocker blocker(&o);
        QVERIFY(o.signalsBlocked());

        o.blockSignals(false);
        QVERIFY(!o.signalsBlocked());

        o.blockSignals(true);
        QVERIFY(o.signalsBlocked());

        blocker.unblock();
        QVERIFY(!o.signalsBlocked());

        blocker.reblock();
        QVERIFY(o.signalsBlocked());
    }

    QVERIFY(!o.signalsBlocked());
}

void tst_QSignalBlocker::moveAssignment()
{
    QObject o1, o2;

    // move-assignment: both block other objects
    {
        QSignalBlocker b(&o1);
        QVERIFY(o1.signalsBlocked());

        QVERIFY(!o2.signalsBlocked());
        b = QSignalBlocker(&o2);
        QVERIFY(!o1.signalsBlocked());
        QVERIFY(o2.signalsBlocked());
    }

    QVERIFY(!o1.signalsBlocked());
    QVERIFY(!o2.signalsBlocked());

    // move-assignment: from inert other
    {
        QSignalBlocker b(&o1);
        QVERIFY(o1.signalsBlocked());
        b = QSignalBlocker(0);
    }

    QVERIFY(!o1.signalsBlocked());
    QVERIFY(!o2.signalsBlocked());

    // move-assignment: to inert *this
    {
        QSignalBlocker b(0);
        QVERIFY(!o1.signalsBlocked());
        {
            QSignalBlocker inner(&o1);
            QVERIFY(o1.signalsBlocked());
            b = std::move(inner);
        }
        QVERIFY(o1.signalsBlocked());
    }

    QVERIFY(!o1.signalsBlocked());
    QVERIFY(!o2.signalsBlocked());

    // move-assignment: both block the same object, neither is unblocked
    {
        QSignalBlocker b(&o1);
        QVERIFY(o1.signalsBlocked());
        {
            b.unblock(); // make sure inner.m_blocked = false
            QVERIFY(!o1.signalsBlocked());
            QSignalBlocker inner(&o1);
            QVERIFY(o1.signalsBlocked());
            b.reblock();
            QVERIFY(o1.signalsBlocked());
            b = std::move(inner);
        }
        QVERIFY(o1.signalsBlocked());
    }

    QVERIFY(!o1.signalsBlocked());
    QVERIFY(!o2.signalsBlocked());

    // move-assignment: both block the same object, but *this is unblocked
    {
        QSignalBlocker b(&o1);
        QVERIFY(o1.signalsBlocked());
        b.unblock();
        QVERIFY(!o1.signalsBlocked());
        b = QSignalBlocker(&o1);
        QVERIFY(o1.signalsBlocked());
    }

    QVERIFY(!o1.signalsBlocked());
    QVERIFY(!o2.signalsBlocked());

    // move-assignment: both block the same object, but other is unblocked
    {
        QSignalBlocker b(&o1);
        {
            QVERIFY(o1.signalsBlocked());
            QSignalBlocker inner(&o1);
            QVERIFY(o1.signalsBlocked());
            inner.unblock();
            QVERIFY(o1.signalsBlocked());
            b = std::move(inner);
            QVERIFY(!o1.signalsBlocked());
        }
        QVERIFY(!o1.signalsBlocked());
    }

    QVERIFY(!o1.signalsBlocked());
    QVERIFY(!o2.signalsBlocked());
}

QTEST_MAIN(tst_QSignalBlocker)
#include "tst_qsignalblocker.moc"
