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

#include <QtTest/QtTest>
#include <qmargins.h>

Q_DECLARE_METATYPE(QMargins)

class tst_QMargins : public QObject
{
    Q_OBJECT
private slots:
    void getSetCheck();
    void dataStreamCheck();
    void operators();
};

// Testing get/set functions
void tst_QMargins::getSetCheck()
{
    QMargins margins;
    // int QMargins::width()
    // void QMargins::setWidth(int)
    margins.setLeft(0);
    QCOMPARE(0, margins.left());
    margins.setTop(INT_MIN);
    QCOMPARE(INT_MIN, margins.top());
    margins.setBottom(INT_MAX);
    QCOMPARE(INT_MAX, margins.bottom());
    margins.setRight(INT_MAX);
    QCOMPARE(INT_MAX, margins.right());

    margins = QMargins();
    QVERIFY(margins.isNull());
    margins.setLeft(5);
    margins.setRight(5);
    QVERIFY(!margins.isNull());
    QCOMPARE(margins, QMargins(5, 0, 5, 0));
}

void tst_QMargins::operators()
{
    const QMargins m1(12, 14, 16, 18);
    const QMargins m2(2, 3, 4, 5);

    const QMargins added = m1 + m2;
    QCOMPARE(added, QMargins(14, 17, 20, 23));
    QMargins a = m1;
    a += m2;
    QCOMPARE(a, added);

    const QMargins subtracted = m1 - m2;
    QCOMPARE(subtracted, QMargins(10, 11, 12, 13));
    a = m1;
    a -= m2;
    QCOMPARE(a, subtracted);

    const QMargins doubled = m1 * 2;
    QCOMPARE(doubled, QMargins(24, 28, 32, 36));
    QCOMPARE(2 * m1, doubled);
    QCOMPARE(qreal(2) * m1, doubled);
    QCOMPARE(m1 * qreal(2), doubled);

    a = m1;
    a *= 2;
    QCOMPARE(a, doubled);
    a = m1;
    a *= qreal(2);
    QCOMPARE(a, doubled);

    const QMargins halved = m1 / 2;
    QCOMPARE(halved, QMargins(6, 7, 8, 9));

    a = m1;
    a /= 2;
    QCOMPARE(a, halved);
    a = m1;
    a /= qreal(2);
    QCOMPARE(a, halved);

    QCOMPARE(m1 + (-m1), QMargins());
}

// Testing QDataStream operators
void tst_QMargins::dataStreamCheck()
{
    QByteArray buffer;

    // stream out
    {
        QMargins marginsOut(0,INT_MIN,INT_MAX,6852);
        QDataStream streamOut(&buffer, QIODevice::WriteOnly);
        streamOut << marginsOut;
    }

    // stream in & compare
    {
        QMargins marginsIn;
        QDataStream streamIn(&buffer, QIODevice::ReadOnly);
        streamIn >> marginsIn;

        QCOMPARE(marginsIn.left(), 0);
        QCOMPARE(marginsIn.top(), INT_MIN);
        QCOMPARE(marginsIn.right(), INT_MAX);
        QCOMPARE(marginsIn.bottom(), 6852);
    }
}

QTEST_APPLESS_MAIN(tst_QMargins)
#include "tst_qmargins.moc"
