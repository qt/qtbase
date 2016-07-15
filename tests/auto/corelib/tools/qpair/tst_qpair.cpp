/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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

#include <QPair>
#include <QSize>

class tst_QPair : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void pairOfReferences();
    void testConstexpr();
    void testConversions();
    void taskQTBUG_48780_pairContainingCArray();
};

class C { char _[4]; };
class M { char _[4]; };
class P { char _[4]; };

QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(M, Q_MOVABLE_TYPE);
Q_DECLARE_TYPEINFO(P, Q_PRIMITIVE_TYPE);
QT_END_NAMESPACE

// avoid the comma:
typedef QPair<C,C> QPairCC;
typedef QPair<C,M> QPairCM;
typedef QPair<C,P> QPairCP;
typedef QPair<M,C> QPairMC;
typedef QPair<M,M> QPairMM;
typedef QPair<M,P> QPairMP;
typedef QPair<P,C> QPairPC;
typedef QPair<P,M> QPairPM;
typedef QPair<P,P> QPairPP;

Q_STATIC_ASSERT( QTypeInfo<QPairCC>::isComplex);
Q_STATIC_ASSERT( QTypeInfo<QPairCC>::isStatic );

Q_STATIC_ASSERT( QTypeInfo<QPairCM>::isComplex);
Q_STATIC_ASSERT( QTypeInfo<QPairCM>::isStatic );

Q_STATIC_ASSERT( QTypeInfo<QPairCP>::isComplex);
Q_STATIC_ASSERT( QTypeInfo<QPairCP>::isStatic );

Q_STATIC_ASSERT( QTypeInfo<QPairMC>::isComplex);
Q_STATIC_ASSERT( QTypeInfo<QPairMC>::isStatic );

Q_STATIC_ASSERT( QTypeInfo<QPairMM>::isComplex);
Q_STATIC_ASSERT(!QTypeInfo<QPairMM>::isStatic );

Q_STATIC_ASSERT( QTypeInfo<QPairMP>::isComplex);
Q_STATIC_ASSERT(!QTypeInfo<QPairMP>::isStatic );

Q_STATIC_ASSERT( QTypeInfo<QPairPC>::isComplex);
Q_STATIC_ASSERT( QTypeInfo<QPairPC>::isStatic );

Q_STATIC_ASSERT( QTypeInfo<QPairPM>::isComplex);
Q_STATIC_ASSERT(!QTypeInfo<QPairPM>::isStatic );

Q_STATIC_ASSERT(!QTypeInfo<QPairPP>::isComplex);
Q_STATIC_ASSERT(!QTypeInfo<QPairPP>::isStatic );

Q_STATIC_ASSERT(!QTypeInfo<QPairPP>::isDummy  );
Q_STATIC_ASSERT(!QTypeInfo<QPairPP>::isPointer);


void tst_QPair::pairOfReferences()
{
    int i = 0;
    QString s;

    QPair<int&, QString&> p(i, s);

    p.first = 1;
    QCOMPARE(i, 1);

    i = 2;
    QCOMPARE(p.first, 2);

    p.second = QLatin1String("Hello");
    QCOMPARE(s, QLatin1String("Hello"));

    s = QLatin1String("olleH");
    QCOMPARE(p.second, QLatin1String("olleH"));

    QPair<int&, QString&> q = p;
    q.first = 3;
    QCOMPARE(i, 3);
    QCOMPARE(p.first, 3);

    q.second = QLatin1String("World");
    QCOMPARE(s, QLatin1String("World"));
    QCOMPARE(p.second, QLatin1String("World"));
}

void tst_QPair::testConstexpr()
{
    Q_CONSTEXPR QPair<int, double> pID = qMakePair(0, 0.0);
    Q_UNUSED(pID);

    Q_CONSTEXPR QPair<double, double> pDD  = qMakePair(0.0, 0.0);
    Q_CONSTEXPR QPair<double, double> pDD2 = qMakePair(0, 0.0);   // involes (rvalue) conversion ctor
    Q_CONSTEXPR bool equal = pDD2 == pDD;
    QVERIFY(equal);

    Q_CONSTEXPR QPair<QSize, int> pSI = qMakePair(QSize(4, 5), 6);
    Q_UNUSED(pSI);
}

void tst_QPair::testConversions()
{
    // construction from lvalue:
    {
        const QPair<int, double> rhs(42, 4.5);
        const QPair<int, int> pii = rhs;
        QCOMPARE(pii.first, 42);
        QCOMPARE(pii.second, 4);

        const QPair<int, float> pif = rhs;
        QCOMPARE(pif.first, 42);
        QCOMPARE(pif.second, 4.5f);
    }

    // assignment from lvalue:
    {
        const QPair<int, double> rhs(42, 4.5);
        QPair<int, int> pii;
        pii = rhs;
        QCOMPARE(pii.first, 42);
        QCOMPARE(pii.second, 4);

        QPair<int, float> pif;
        pif = rhs;
        QCOMPARE(pif.first, 42);
        QCOMPARE(pif.second, 4.5f);
    }

    // construction from rvalue:
    {
#define rhs qMakePair(42, 4.5)
        const QPair<int, int> pii = rhs;
        QCOMPARE(pii.first, 42);
        QCOMPARE(pii.second, 4);

        const QPair<int, float> pif = rhs;
        QCOMPARE(pif.first, 42);
        QCOMPARE(pif.second, 4.5f);
#undef rhs
    }

    // assignment from rvalue:
    {
#define rhs qMakePair(42, 4.5)
        QPair<int, int> pii;
        pii = rhs;
        QCOMPARE(pii.first, 42);
        QCOMPARE(pii.second, 4);

        QPair<int, float> pif;
        pif = rhs;
        QCOMPARE(pif.first, 42);
        QCOMPARE(pif.second, 4.5f);
#undef rhs
    }
}

void tst_QPair::taskQTBUG_48780_pairContainingCArray()
{
    // compile-only:
    QPair<int[2], int> pair;
    pair.first[0] = 0;
    pair.first[1] = 1;
    pair.second = 2;
    Q_UNUSED(pair);
}

QTEST_APPLESS_MAIN(tst_QPair)
#include "tst_qpair.moc"
