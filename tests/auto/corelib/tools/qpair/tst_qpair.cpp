/****************************************************************************
**
** Copyright (C) 2012 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
** Contact: http://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
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
    void testConstexpr();
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

QTEST_APPLESS_MAIN(tst_QPair)
#include "tst_qpair.moc"
