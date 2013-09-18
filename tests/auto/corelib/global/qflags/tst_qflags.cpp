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

class tst_QFlags: public QObject
{
    Q_OBJECT
private slots:
    void testFlag() const;
    void testFlagZeroFlag() const;
    void testFlagMultiBits() const;
    void constExpr();
    void signedness();
    void classEnum();
};

void tst_QFlags::testFlag() const
{
    Qt::MouseButtons btn = Qt::LeftButton | Qt::RightButton;

    QVERIFY(btn.testFlag(Qt::LeftButton));
    QVERIFY(!btn.testFlag(Qt::MidButton));

    btn = 0;
    QVERIFY(!btn.testFlag(Qt::LeftButton));
}

void tst_QFlags::testFlagZeroFlag() const
{
    {
        Qt::MouseButtons btn = Qt::LeftButton | Qt::RightButton;
        /* Qt::NoButton has the value 0. */

        QVERIFY(!btn.testFlag(Qt::NoButton));
    }

    {
        /* A zero enum set should test true with zero. */
        QVERIFY(Qt::MouseButtons().testFlag(Qt::NoButton));
    }

    {
        Qt::MouseButtons btn = Qt::NoButton;
        QVERIFY(btn.testFlag(Qt::NoButton));
    }
}

void tst_QFlags::testFlagMultiBits() const
{
    /* Qt::Window is 0x00000001
     * Qt::Dialog is 0x00000002 | Window
     */
    {
        const Qt::WindowFlags onlyWindow(Qt::Window);
        QVERIFY(!onlyWindow.testFlag(Qt::Dialog));
    }

    {
        const Qt::WindowFlags hasDialog(Qt::Dialog);
        QVERIFY(hasDialog.testFlag(Qt::Dialog));
    }
}

template <unsigned int N, typename T> bool verifyConstExpr(T n) { return n == N; }

void tst_QFlags::constExpr()
{
#ifdef Q_COMPILER_CONSTEXPR
    Qt::MouseButtons btn = Qt::LeftButton | Qt::RightButton;
    switch (btn) {
        case Qt::LeftButton: QVERIFY(false); break;
        case Qt::RightButton: QVERIFY(false); break;
        case int(Qt::LeftButton | Qt::RightButton): QVERIFY(true); break;
        default: QVERIFY(false);
    }

    QVERIFY(verifyConstExpr<(Qt::LeftButton | Qt::RightButton) & Qt::LeftButton>(Qt::LeftButton));
    QVERIFY(verifyConstExpr<(Qt::LeftButton | Qt::RightButton) & Qt::MiddleButton>(0));
    QVERIFY(verifyConstExpr<(Qt::LeftButton | Qt::RightButton) | Qt::MiddleButton>(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton));
    QVERIFY(verifyConstExpr<~(Qt::LeftButton | Qt::RightButton)>(~(Qt::LeftButton | Qt::RightButton)));
    QVERIFY(verifyConstExpr<Qt::MouseButtons(Qt::LeftButton) ^ Qt::RightButton>(Qt::LeftButton ^ Qt::RightButton));
    QVERIFY(verifyConstExpr<Qt::MouseButtons(0)>(0));
    QVERIFY(verifyConstExpr<Qt::MouseButtons(Qt::RightButton) & 0xff>(Qt::RightButton));
    QVERIFY(verifyConstExpr<Qt::MouseButtons(Qt::RightButton) | 0xff>(0xff));

    QVERIFY(!verifyConstExpr<Qt::RightButton>(!Qt::MouseButtons(Qt::LeftButton)));
#endif
}

void tst_QFlags::signedness()
{
    // these are all 'true' on GCC, but since the std says the
    // underlying type is implementation-defined, we need to allow for
    // a different signedness, so we only check that the relative
    // signedness of the types matches:
    Q_STATIC_ASSERT((QtPrivate::is_unsigned<Qt::MouseButton>::value ==
                     QtPrivate::is_unsigned<Qt::MouseButtons::Int>::value));

    Q_STATIC_ASSERT((QtPrivate::is_signed<Qt::AlignmentFlag>::value ==
                     QtPrivate::is_signed<Qt::Alignment::Int>::value));
}

#if defined(Q_COMPILER_CLASS_ENUM)
enum class MyStrictEnum { StrictZero, StrictOne, StrictTwo, StrictFour=4 };
Q_DECLARE_FLAGS( MyStrictFlags, MyStrictEnum )
Q_DECLARE_OPERATORS_FOR_FLAGS( MyStrictFlags )

Q_STATIC_ASSERT( !QTypeInfo<MyStrictFlags>::isComplex );
Q_STATIC_ASSERT( !QTypeInfo<MyStrictFlags>::isStatic );
Q_STATIC_ASSERT( !QTypeInfo<MyStrictFlags>::isLarge );
Q_STATIC_ASSERT( !QTypeInfo<MyStrictFlags>::isPointer );
#endif

void tst_QFlags::classEnum()
{
#if defined(Q_COMPILER_CLASS_ENUM)
    // The main aim of the test is making sure it compiles
    // The QCOMPARE are there as an extra
    MyStrictEnum e1 = MyStrictEnum::StrictOne;
    MyStrictEnum e2 = MyStrictEnum::StrictTwo;

    MyStrictFlags f1(MyStrictEnum::StrictOne);
    QCOMPARE(f1, 1);

    MyStrictFlags f2(e2);
    QCOMPARE(f2, 2);

    MyStrictFlags f0;
    QCOMPARE(f0, 0);

    MyStrictFlags f3(e2 | e1);
    QCOMPARE(f3, 3);

    QVERIFY(f3.testFlag(MyStrictEnum::StrictOne));
    QVERIFY(!f1.testFlag(MyStrictEnum::StrictTwo));

    QVERIFY(!f0);

    QCOMPARE(f3 & int(1), 1);
    QCOMPARE(f3 & uint(1), 1);
    QCOMPARE(f3 & MyStrictEnum::StrictOne, 1);

    MyStrictFlags aux;
    aux = f3;
    aux &= int(1);
    QCOMPARE(aux, 1);

    aux = f3;
    aux &= uint(1);
    QCOMPARE(aux, 1);

    aux = f3;
    aux &= MyStrictEnum::StrictOne;
    QCOMPARE(aux, 1);

    aux = f3;
    aux &= f1;
    QCOMPARE(aux, 1);

    aux = f3 ^ f3;
    QCOMPARE(aux, 0);

    aux = f3 ^ f1;
    QCOMPARE(aux, 2);

    aux = f3 ^ f0;
    QCOMPARE(aux, 3);

    aux = f3 ^ MyStrictEnum::StrictOne;
    QCOMPARE(aux, 2);

    aux = f3 ^ MyStrictEnum::StrictZero;
    QCOMPARE(aux, 3);

    aux = f3;
    aux ^= f3;
    QCOMPARE(aux, 0);

    aux = f3;
    aux ^= f1;
    QCOMPARE(aux, 2);

    aux = f3;
    aux ^= f0;
    QCOMPARE(aux, 3);

    aux = f3;
    aux ^= MyStrictEnum::StrictOne;
    QCOMPARE(aux, 2);

    aux = f3;
    aux ^= MyStrictEnum::StrictZero;
    QCOMPARE(aux, 3);

    aux = f1 | f2;
    QCOMPARE(aux, 3);

    aux = MyStrictEnum::StrictOne | MyStrictEnum::StrictTwo;
    QCOMPARE(aux, 3);

    aux = f1;
    aux |= f2;
    QCOMPARE(aux, 3);

    aux = MyStrictEnum::StrictOne;
    aux |= MyStrictEnum::StrictTwo;
    QCOMPARE(aux, 3);

    aux = ~f1;
    QCOMPARE(aux, -2);

    // Just to make sure it compiles
    if (false)
        qDebug() << f3;
#endif
}

// (statically) check QTypeInfo for QFlags instantiations:
enum MyEnum { Zero, One, Two, Four=4 };
Q_DECLARE_FLAGS( MyFlags, MyEnum )
Q_DECLARE_OPERATORS_FOR_FLAGS( MyFlags )

Q_STATIC_ASSERT( !QTypeInfo<MyFlags>::isComplex );
Q_STATIC_ASSERT( !QTypeInfo<MyFlags>::isStatic );
Q_STATIC_ASSERT( !QTypeInfo<MyFlags>::isLarge );
Q_STATIC_ASSERT( !QTypeInfo<MyFlags>::isPointer );

QTEST_MAIN(tst_QFlags)
#include "tst_qflags.moc"
