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
