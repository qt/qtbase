/****************************************************************************
**
** Copyright (C) 2015 Robin Burchell <robin.burchell@viroteck.net>
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

#include <QStack>
#include <QDebug>
#include <QtTest>

#include <vector>

class tst_QStack: public QObject
{
    Q_OBJECT

private slots:
    void qstack_push();
    void qstack_pop();
    void qstack_pushpopone();
};

const int N = 1000000;

void tst_QStack::qstack_push()
{
    QStack<int> v;
    QBENCHMARK {
        for (int i = 0; i != N; ++i)
            v.push(i);
        v = QStack<int>();
    }
}

void tst_QStack::qstack_pop()
{
    QStack<int> v;
    for (int i = 0; i != N; ++i)
        v.push(i);

    QBENCHMARK {
        QStack<int> v2 = v;
        for (int i = 0; i != N; ++i) {
            v2.pop();
        }
    }
}

void tst_QStack::qstack_pushpopone()
{
    QBENCHMARK {
        QStack<int> v;
        for (int i = 0; i != N; ++i) {
            v.push(0);
            v.pop();
        }
    }
}

QTEST_MAIN(tst_QStack)

#include "main.moc"
