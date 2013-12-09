/****************************************************************************
**
** Copyright (C) 2013 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@woboq.com>
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

#include "qobject.h"

class tst_QSignalBlocker : public QObject
{
    Q_OBJECT
private slots:
    void signalBlocking();
    void signalBlockingMoveAssignment();
};

class SenderObject : public QObject
{
    Q_OBJECT

public:
    SenderObject() : aPublicSlotCalled(0), recursionCount(0) {}

    void emitSignal1AfterRecursion()
    {
        if (recursionCount++ < 100)
            emitSignal1AfterRecursion();
        else
            emitSignal1();
    }

    void emitSignal1() { emit signal1(); }
    void emitSignal2() { emit signal2(); }
    void emitSignal3() { emit signal3(); }
    void emitSignal4() { emit signal4(); }

signals:
    void signal1();
    void signal2();
    void signal3();
    void signal4();
    QT_MOC_COMPAT void signal5();
    void signal6(void);
    void signal7(int, const QString &);

public slots:
    void aPublicSlot() { aPublicSlotCalled++; }

public:
    Q_INVOKABLE void invoke1(){}
    Q_SCRIPTABLE void sinvoke1(){}
    int aPublicSlotCalled;
protected:
    Q_INVOKABLE QT_MOC_COMPAT void invoke2(){}
    Q_INVOKABLE QT_MOC_COMPAT void invoke2(int){}
    Q_SCRIPTABLE QT_MOC_COMPAT void sinvoke2(){}
private:
    Q_INVOKABLE void invoke3(int hinz = 0, int kunz = 0){Q_UNUSED(hinz) Q_UNUSED(kunz)}
    Q_SCRIPTABLE void sinvoke3(){}

    int recursionCount;
};

class ReceiverObject : public QObject
{
    Q_OBJECT

public:
    ReceiverObject()
        : sequence_slot1( 0 )
        , sequence_slot2( 0 )
        , sequence_slot3( 0 )
        , sequence_slot4( 0 )
    {}

    void reset()
    {
        sequence_slot4 = 0;
        sequence_slot3 = 0;
        sequence_slot2 = 0;
        sequence_slot1 = 0;
        count_slot1 = 0;
        count_slot2 = 0;
        count_slot3 = 0;
        count_slot4 = 0;
    }

    int sequence_slot1;
    int sequence_slot2;
    int sequence_slot3;
    int sequence_slot4;
    int count_slot1;
    int count_slot2;
    int count_slot3;
    int count_slot4;

    bool called(int slot)
    {
        switch (slot) {
        case 1: return sequence_slot1;
        case 2: return sequence_slot2;
        case 3: return sequence_slot3;
        case 4: return sequence_slot4;
        default: return false;
        }
    }

    static int sequence;

public slots:
    void slot1() { sequence_slot1 = ++sequence; count_slot1++; }
    void slot2() { sequence_slot2 = ++sequence; count_slot2++; }
    void slot3() { sequence_slot3 = ++sequence; count_slot3++; }
    void slot4() { sequence_slot4 = ++sequence; count_slot4++; }

};

int ReceiverObject::sequence = 0;

void tst_QSignalBlocker::signalBlocking()
{
    SenderObject sender;
    ReceiverObject receiver;

    receiver.connect(&sender, SIGNAL(signal1()), SLOT(slot1()));

    sender.emitSignal1();
    QVERIFY(receiver.called(1));
    receiver.reset();

    {
        QSignalBlocker blocker(&sender);

        sender.emitSignal1();
        QVERIFY(!receiver.called(1));
        receiver.reset();

        sender.blockSignals(false);

        sender.emitSignal1();
        QVERIFY(receiver.called(1));
        receiver.reset();

        sender.blockSignals(true);

        sender.emitSignal1();
        QVERIFY(!receiver.called(1));
        receiver.reset();

        blocker.unblock();

        sender.emitSignal1();
        QVERIFY(receiver.called(1));
        receiver.reset();

        blocker.reblock();

        sender.emitSignal1();
        QVERIFY(!receiver.called(1));
        receiver.reset();
    }

    sender.emitSignal1();
    QVERIFY(receiver.called(1));
    receiver.reset();
}

void tst_QSignalBlocker::signalBlockingMoveAssignment()
{
#ifdef Q_COMPILER_RVALUE_REFS
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

#else
    QSKIP("This compiler is not in C++11 mode or doesn't support move semantics");
#endif // Q_COMPILER_RVALUE_REFS
}

QTEST_MAIN(tst_QSignalBlocker)
#include "tst_qsignalblocker.moc"
