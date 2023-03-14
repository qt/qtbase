// Copyright (C) 2022 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QMargins>
#ifdef QVARIANT_H
# error "This test requires qmargins.h to not include qvariant.h"
#endif

// don't assume <type_traits>
template <typename T, typename U>
constexpr inline bool my_is_same_v = false;
template <typename T>
constexpr inline bool my_is_same_v<T, T> = true;

#define CHECK(cvref) \
    static_assert(my_is_same_v<decltype(get<0>(std::declval<QMargins cvref >())), int cvref >); \
    static_assert(my_is_same_v<decltype(get<1>(std::declval<QMargins cvref >())), int cvref >); \
    static_assert(my_is_same_v<decltype(get<2>(std::declval<QMargins cvref >())), int cvref >); \
    static_assert(my_is_same_v<decltype(get<3>(std::declval<QMargins cvref >())), int cvref >); \
    \
    static_assert(my_is_same_v<decltype(get<0>(std::declval<QMarginsF cvref >())), qreal cvref >); \
    static_assert(my_is_same_v<decltype(get<1>(std::declval<QMarginsF cvref >())), qreal cvref >); \
    static_assert(my_is_same_v<decltype(get<2>(std::declval<QMarginsF cvref >())), qreal cvref >); \
    static_assert(my_is_same_v<decltype(get<3>(std::declval<QMarginsF cvref >())), qreal cvref >)

CHECK(&);
CHECK(const &);
CHECK(&&);
CHECK(const &&);

#undef CHECK

#include <QTest>
#include <qmargins.h>

#include <array>

Q_DECLARE_METATYPE(QMargins)

class tst_QMargins : public QObject
{
    Q_OBJECT
private slots:
    void getSetCheck();
#ifndef QT_NO_DATASTREAM
    void dataStreamCheck();
#endif
    void operators();
#ifndef QT_NO_DEBUG_STREAM
    void debugStreamCheck();
#endif

    void getSetCheckF();
#ifndef QT_NO_DATASTREAM
    void dataStreamCheckF();
#endif
    void operatorsF();
#ifndef QT_NO_DEBUG_STREAM
    void debugStreamCheckF();
#endif

    void structuredBinding();

    void toMarginsF_data();
    void toMarginsF();
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

    QMargins h = m1;
    h += 2;
    QCOMPARE(h, QMargins(14, 16, 18, 20));
    h -= 2;
    QCOMPARE(h, m1);

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

    QMargins m3 = QMargins(10, 11, 12, 13);
    QCOMPARE(m3 + 1, QMargins(11, 12, 13, 14));
    QCOMPARE(1 + m3, QMargins(11, 12, 13, 14));
    QCOMPARE(m3 - 1, QMargins(9, 10, 11, 12));
    QCOMPARE(+m3, QMargins(10, 11, 12, 13));
    QCOMPARE(-m3, QMargins(-10, -11, -12, -13));
}

#ifndef QT_NO_DEBUG_STREAM
// Testing QDebug operators
void tst_QMargins::debugStreamCheck()
{
    QMargins m(10, 11, 12, 13);
    const QString expected = "QMargins(10, 11, 12, 13)";
    QString result;
    QDebug(&result).nospace() << m;
    QCOMPARE(result, expected);
}
#endif

#ifndef QT_NO_DATASTREAM
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
#endif

// Testing get/set functions
void tst_QMargins::getSetCheckF()
{
    QMarginsF margins;
    // int QMarginsF::width()
    // void QMarginsF::setWidth(int)
    margins.setLeft(1.1);
    QCOMPARE(1.1, margins.left());
    margins.setTop(2.2);
    QCOMPARE(2.2, margins.top());
    margins.setBottom(3.3);
    QCOMPARE(3.3, margins.bottom());
    margins.setRight(4.4);
    QCOMPARE(4.4, margins.right());

    margins = QMarginsF();
    QVERIFY(margins.isNull());
    margins.setLeft(5.5);
    margins.setRight(5.5);
    QVERIFY(!margins.isNull());
    QCOMPARE(margins, QMarginsF(5.5, 0.0, 5.5, 0.0));
}

void tst_QMargins::operatorsF()
{
    const QMarginsF m1(12.1, 14.1, 16.1, 18.1);
    const QMarginsF m2(2.1, 3.1, 4.1, 5.1);

    const QMarginsF added = m1 + m2;
    QCOMPARE(added, QMarginsF(14.2, 17.2, 20.2, 23.2));
    QMarginsF a = m1;
    a += m2;
    QCOMPARE(a, added);

    const QMarginsF subtracted = m1 - m2;
    QCOMPARE(subtracted, QMarginsF(10.0, 11.0, 12.0, 13.0));
    a = m1;
    a -= m2;
    QCOMPARE(a, subtracted);

    QMarginsF h = m1;
    h += 2.1;
    QCOMPARE(h, QMarginsF(14.2, 16.2, 18.2, 20.2));
    h -= 2.1;
    QCOMPARE(h, m1);

    const QMarginsF doubled = m1 * 2.0;
    QCOMPARE(doubled, QMarginsF(24.2, 28.2, 32.2, 36.2));
    QCOMPARE(2.0 * m1, doubled);
    QCOMPARE(m1 * 2.0, doubled);

    a = m1;
    a *= 2.0;
    QCOMPARE(a, doubled);

    const QMarginsF halved = m1 / 2.0;
    QCOMPARE(halved, QMarginsF(6.05, 7.05, 8.05, 9.05));

    a = m1;
    a /= 2.0;
    QCOMPARE(a, halved);

    QCOMPARE(m1 + (-m1), QMarginsF());

    QMarginsF m3 = QMarginsF(10.3, 11.4, 12.5, 13.6);
    QCOMPARE(m3 + 1.1, QMarginsF(11.4, 12.5, 13.6, 14.7));
    QCOMPARE(1.1 + m3, QMarginsF(11.4, 12.5, 13.6, 14.7));
    QCOMPARE(m3 - 1.1, QMarginsF(9.2, 10.3, 11.4, 12.5));
    QCOMPARE(+m3, QMarginsF(10.3, 11.4, 12.5, 13.6));
    QCOMPARE(-m3, QMarginsF(-10.3, -11.4, -12.5, -13.6));
}

#ifndef QT_NO_DATASTREAM
// Testing QDataStream operators
void tst_QMargins::dataStreamCheckF()
{
    QByteArray buffer;

    // stream out
    {
        QMarginsF marginsOut(1.1, 2.2, 3.3, 4.4);
        QDataStream streamOut(&buffer, QIODevice::WriteOnly);
        streamOut << marginsOut;
    }

    // stream in & compare
    {
        QMarginsF marginsIn;
        QDataStream streamIn(&buffer, QIODevice::ReadOnly);
        streamIn >> marginsIn;

        QCOMPARE(marginsIn.left(), 1.1);
        QCOMPARE(marginsIn.top(), 2.2);
        QCOMPARE(marginsIn.right(), 3.3);
        QCOMPARE(marginsIn.bottom(), 4.4);
    }
}
#endif

#ifndef QT_NO_DEBUG_STREAM
// Testing QDebug operators
void tst_QMargins::debugStreamCheckF()
{
    QMarginsF m(10.1, 11.2, 12.3, 13.4);
    const QString expected = "QMarginsF(10.1, 11.2, 12.3, 13.4)";
    QString result;
    QDebug(&result).nospace() << m;
    QCOMPARE(result, expected);
}
#endif

void tst_QMargins::structuredBinding()
{
    {
        QMargins m(1, 2, 3, 4);
        auto [left, top, right, bottom] = m;
        QCOMPARE(left, 1);
        QCOMPARE(top, 2);
        QCOMPARE(right, 3);
        QCOMPARE(bottom, 4);
    }
    {
        QMargins m(1, 2, 3, 4);
        auto &[left, top, right, bottom] = m;
        QCOMPARE(left, 1);
        QCOMPARE(top, 2);
        QCOMPARE(right, 3);
        QCOMPARE(bottom, 4);

        left = 10;
        top = 20;
        right = 30;
        bottom = 40;
        QCOMPARE(m.left(), 10);
        QCOMPARE(m.top(), 20);
        QCOMPARE(m.right(), 30);
        QCOMPARE(m.bottom(), 40);
    }
    {
        QMarginsF m(1.0, 2.0, 3.0, 4.0);
        auto [left, top, right, bottom] = m;
        QCOMPARE(left, 1.0);
        QCOMPARE(top, 2.0);
        QCOMPARE(right, 3.0);
        QCOMPARE(bottom, 4.0);
    }
    {
        QMarginsF m(1.0, 2.0, 3.0, 4.0);
        auto &[left, top, right, bottom] = m;
        QCOMPARE(left, 1.0);
        QCOMPARE(top, 2.0);
        QCOMPARE(right, 3.0);
        QCOMPARE(bottom, 4.0);

        left = 10.0;
        top = 20.0;
        right = 30.0;
        bottom = 40.0;
        QCOMPARE(m.left(), 10.0);
        QCOMPARE(m.top(), 20.0);
        QCOMPARE(m.right(), 30.0);
        QCOMPARE(m.bottom(), 40.0);
    }
}

void tst_QMargins::toMarginsF_data()
{
    QTest::addColumn<QMargins>("input");
    QTest::addColumn<QMarginsF>("result");

    auto row = [](int x1, int y1, int x2, int y2) {
        QTest::addRow("(%d, %d, %d, %d)", x1, y1, x2, y2)
                << QMargins(x1, y1, x2, y2) << QMarginsF(x1, y1, x2, y2);
    };
    constexpr std::array samples = {-1, 0, 1};
    for (int x1 : samples) {
        for (int y1 : samples) {
            for (int x2 : samples) {
                for (int y2 : samples) {
                    row(x1, y1, x2, y2);
                }
            }
        }
    }
}

void tst_QMargins::toMarginsF()
{
    QFETCH(const QMargins, input);
    QFETCH(const QMarginsF, result);

    QCOMPARE(input.toMarginsF(), result);
}

QTEST_APPLESS_MAIN(tst_QMargins)
#include "tst_qmargins.moc"
