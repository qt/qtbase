// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QObject>
#include <QTest>

class tst_QKeyCombination : public QObject
{
    Q_OBJECT
private slots:
    void construction();
    void operator_eq();
    void operator_or();
};

constexpr int bitwiseOr()
{
    return 0;
}

template <typename ...T>
constexpr auto bitwiseOr(T ... args)
{
    return (... | ((int)args));
}

void tst_QKeyCombination::construction()
{
    {
        QKeyCombination combination;
        QCOMPARE(combination.key(), Qt::Key_unknown);
        QCOMPARE(combination.keyboardModifiers(), Qt::KeyboardModifiers{});
        QCOMPARE(combination.toCombined(), bitwiseOr(Qt::Key_unknown));
    }

    {
        QKeyCombination combination(Qt::CTRL); // explicit
        QCOMPARE(combination.key(), Qt::Key_unknown);
        QCOMPARE(combination.keyboardModifiers(), Qt::KeyboardModifiers(Qt::ControlModifier));
        QCOMPARE(combination.toCombined(), bitwiseOr(Qt::ControlModifier, Qt::Key_unknown));
    }

    {
        QKeyCombination combination(Qt::SHIFT); // explicit
        QCOMPARE(combination.key(), Qt::Key_unknown);
        QCOMPARE(combination.keyboardModifiers(), Qt::KeyboardModifiers(Qt::ShiftModifier));
        QCOMPARE(combination.toCombined(), bitwiseOr(Qt::ShiftModifier, Qt::Key_unknown));
    }

    {
        QKeyCombination combination(Qt::AltModifier); // explicit
        QCOMPARE(combination.key(), Qt::Key_unknown);
        QCOMPARE(combination.keyboardModifiers(), Qt::KeyboardModifiers(Qt::AltModifier));
        QCOMPARE(combination.toCombined(), bitwiseOr(Qt::AltModifier, Qt::Key_unknown));
    }

    {
        QKeyCombination combination(Qt::AltModifier | Qt::ControlModifier); // explicit
        QCOMPARE(combination.key(), Qt::Key_unknown);
        QCOMPARE(combination.keyboardModifiers(), Qt::KeyboardModifiers(Qt::AltModifier | Qt::ControlModifier));
        QCOMPARE(combination.toCombined(), bitwiseOr(Qt::AltModifier, Qt::ControlModifier, Qt::Key_unknown));
    }

    {
        QKeyCombination combination = Qt::Key_A; // implicit
        QCOMPARE(combination.key(), Qt::Key_A);
        QCOMPARE(combination.keyboardModifiers(), Qt::KeyboardModifiers{});
        QCOMPARE(combination.toCombined(), bitwiseOr(Qt::Key_A));
    }

    {
        QKeyCombination combination = Qt::Key_F1; // implicit
        QCOMPARE(combination.key(), Qt::Key_F1);
        QCOMPARE(combination.keyboardModifiers(), Qt::KeyboardModifiers{});
        QCOMPARE(combination.toCombined(), bitwiseOr(Qt::Key_F1));
    }

    {
        QKeyCombination combination(Qt::SHIFT, Qt::Key_F1);
        QCOMPARE(combination.key(), Qt::Key_F1);
        QCOMPARE(combination.keyboardModifiers(), Qt::KeyboardModifiers(Qt::ShiftModifier));
        QCOMPARE(combination.toCombined(), bitwiseOr(Qt::SHIFT, Qt::Key_F1));
    }

    {
        QKeyCombination combination(Qt::SHIFT | Qt::CTRL, Qt::Key_F1);
        QCOMPARE(combination.key(), Qt::Key_F1);
        QCOMPARE(combination.keyboardModifiers(), Qt::KeyboardModifiers(Qt::ShiftModifier | Qt::ControlModifier));
        QCOMPARE(combination.toCombined(), bitwiseOr(Qt::SHIFT, Qt::CTRL, Qt::Key_F1));
    }

    {
        QKeyCombination combination(Qt::AltModifier, Qt::Key_F1);
        QCOMPARE(combination.key(), Qt::Key_F1);
        QCOMPARE(combination.keyboardModifiers(), Qt::KeyboardModifiers(Qt::AltModifier));
        QCOMPARE(combination.toCombined(), bitwiseOr(Qt::AltModifier, Qt::Key_F1));
    }

    // corner cases
    {
        QKeyCombination combination = Qt::Key_Alt;
        QCOMPARE(combination.key(), Qt::Key_Alt);
        QCOMPARE(combination.keyboardModifiers(), Qt::KeyboardModifiers{});
        QCOMPARE(combination.toCombined(), bitwiseOr(Qt::Key_Alt));
    }

    {
        QKeyCombination combination(Qt::ALT, Qt::Key_Alt);
        QCOMPARE(combination.key(), Qt::Key_Alt);
        QCOMPARE(combination.keyboardModifiers(), Qt::KeyboardModifiers(Qt::AltModifier));
        QCOMPARE(combination.toCombined(), bitwiseOr(Qt::ALT, Qt::Key_Alt));
    }

    {
        QKeyCombination combination(Qt::Key_unknown);
        QCOMPARE(combination.key(), Qt::Key_unknown);
        QCOMPARE(combination.keyboardModifiers(), Qt::KeyboardModifiers{});
        QCOMPARE(combination.toCombined(), bitwiseOr(Qt::Key_unknown));
    }

    {
        QKeyCombination combination(Qt::CTRL | Qt::SHIFT, Qt::Key_unknown);
        QCOMPARE(combination.key(), Qt::Key_unknown);
        QCOMPARE(combination.keyboardModifiers(), Qt::KeyboardModifiers(Qt::ControlModifier | Qt::ShiftModifier));
        QCOMPARE(combination.toCombined(), bitwiseOr(Qt::CTRL, Qt::SHIFT, Qt::Key_unknown));
    }
}

void tst_QKeyCombination::operator_eq()
{
    // default
    {
        QKeyCombination a, b;
        QVERIFY(a == b);
        QVERIFY(!(a != b));
    }

    // key only
    {
        QKeyCombination a;
        QKeyCombination b(Qt::Key_X);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }

    {
        QKeyCombination a(Qt::Key_Y);
        QKeyCombination b;
        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }

    {
        QKeyCombination a(Qt::Key_Y);
        QKeyCombination b(Qt::Key_X);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }

    {
        QKeyCombination a(Qt::Key_F1);
        QKeyCombination b(Qt::Key_F1);
        QVERIFY(a == b);
        QVERIFY(!(a != b));
    }

    // modifier only
    {
        QKeyCombination a;
        QKeyCombination b(Qt::CTRL);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }

    {
        QKeyCombination a(Qt::CTRL);
        QKeyCombination b;
        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }

    {
        QKeyCombination a(Qt::CTRL);
        QKeyCombination b(Qt::SHIFT);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }

    {
        QKeyCombination a(Qt::CTRL);
        QKeyCombination b(Qt::CTRL);
        QVERIFY(a == b);
        QVERIFY(!(a != b));
    }

    {
        QKeyCombination a(Qt::CTRL);
        QKeyCombination b(Qt::ControlModifier);
        QVERIFY(a == b);
        QVERIFY(!(a != b));
    }

    {
        QKeyCombination a(Qt::ControlModifier);
        QKeyCombination b(Qt::CTRL);
        QVERIFY(a == b);
        QVERIFY(!(a != b));
    }

    {
        QKeyCombination a(Qt::ControlModifier);
        QKeyCombination b(Qt::ControlModifier);
        QVERIFY(a == b);
        QVERIFY(!(a != b));
    }

    // key and modifier
    {
        QKeyCombination a(Qt::Key_A);
        QKeyCombination b(Qt::SHIFT, Qt::Key_A);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }

    {
        QKeyCombination a(Qt::CTRL, Qt::Key_A);
        QKeyCombination b(Qt::SHIFT, Qt::Key_A);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }

    {
        QKeyCombination a(Qt::SHIFT, Qt::Key_A);
        QKeyCombination b(Qt::SHIFT, Qt::Key_A);
        QVERIFY(a == b);
        QVERIFY(!(a != b));
    }

    {
        QKeyCombination a(Qt::SHIFT, Qt::Key_A);
        QKeyCombination b(Qt::SHIFT, Qt::Key_Escape);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }

    {
        QKeyCombination a(Qt::SHIFT, Qt::Key_A);
        QKeyCombination b(Qt::ShiftModifier, Qt::Key_A);
        QVERIFY(a == b);
        QVERIFY(!(a != b));
    }

    {
        QKeyCombination a(Qt::SHIFT | Qt::CTRL, Qt::Key_A);
        QKeyCombination b(Qt::ControlModifier | Qt::ShiftModifier, Qt::Key_A);
        QVERIFY(a == b);
        QVERIFY(!(a != b));
    }

    // corner cases
    {
        QKeyCombination a(Qt::CTRL);
        QKeyCombination b(Qt::Key_Control);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }

    {
        QKeyCombination a(Qt::ALT);
        QKeyCombination b(Qt::Key_Alt);
        QVERIFY(a != b);
        QVERIFY(!(a == b));
    }
}

void tst_QKeyCombination::operator_or()
{
    // note tha operator+ between enumerators of the same enumeration
    // yields int, so one can't do
    //    Qt::SHIFT + Qt::CTRL + Qt::Key_A
    // but one can do
    //    Qt::SHIFT | Qt::CTRL | Qt::Key_A

    QCOMPARE(Qt::SHIFT | Qt::Key_A, QKeyCombination(Qt::SHIFT, Qt::Key_A));
    QCOMPARE(Qt::AltModifier | Qt::Key_F1, QKeyCombination(Qt::AltModifier, Qt::Key_F1));
    QCOMPARE(Qt::SHIFT | Qt::ALT | Qt::Key_F1, QKeyCombination(Qt::SHIFT | Qt::ALT, Qt::Key_F1));
    QCOMPARE(Qt::ControlModifier | Qt::Key_Escape, QKeyCombination(Qt::ControlModifier, Qt::Key_Escape));
}

QTEST_APPLESS_MAIN(tst_QKeyCombination)

#include "tst_qkeycombination.moc"
