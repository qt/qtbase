/****************************************************************************
**
** Copyright (C) 2015 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Marc Mutz <marc.mutz@kdab.com>
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

#include <QString>

// Preserve QLatin1String-ness (QVariant(QLatin1String) creates a QVariant::String):
struct QLatin1StringContainer {
    QLatin1String l1;
};
QT_BEGIN_NAMESPACE
Q_DECLARE_TYPEINFO(QLatin1StringContainer, Q_MOVABLE_TYPE);
QT_END_NAMESPACE
Q_DECLARE_METATYPE(QLatin1StringContainer)

class tst_QLatin1String : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void nullString();
    void emptyString();
    void relationalOperators_data();
    void relationalOperators();
};

void tst_QLatin1String::nullString()
{
    // default ctor
    {
        QLatin1String l1;
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(Q_NULLPTR));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isNull());
    }

    // from nullptr
    {
        const char *null = Q_NULLPTR;
        QLatin1String l1(null);
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(Q_NULLPTR));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isNull());
    }

    // from null QByteArray
    {
        const QByteArray null;
        QVERIFY(null.isNull());

        QLatin1String l1(null);
        QEXPECT_FAIL("", "null QByteArrays become non-null QLatin1Strings...", Continue);
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(Q_NULLPTR));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QEXPECT_FAIL("", "null QByteArrays become non-null QLatin1Strings become non-null QStrings...", Continue);
        QVERIFY(s.isNull());
    }
}

void tst_QLatin1String::emptyString()
{
    {
        const char *empty = "";
        QLatin1String l1(empty);
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(empty));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isEmpty());
        QVERIFY(!s.isNull());
    }

    {
        const char *notEmpty = "foo";
        QLatin1String l1(notEmpty, 0);
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(notEmpty));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isEmpty());
        QVERIFY(!s.isNull());
    }

    {
        const QByteArray empty = "";
        QLatin1String l1(empty);
        QCOMPARE(static_cast<const void*>(l1.data()), static_cast<const void*>(empty.constData()));
        QCOMPARE(l1.size(), 0);

        QString s = l1;
        QVERIFY(s.isEmpty());
        QVERIFY(!s.isNull());
    }
}

void tst_QLatin1String::relationalOperators_data()
{
    QTest::addColumn<QLatin1StringContainer>("lhs");
    QTest::addColumn<int>("lhsOrderNumber");
    QTest::addColumn<QLatin1StringContainer>("rhs");
    QTest::addColumn<int>("rhsOrderNumber");

    struct Data {
        QLatin1String l1;
        int order;
    } data[] = {
        { QLatin1String(),     0 },
        { QLatin1String(""),   0 },
        { QLatin1String("a"),  1 },
        { QLatin1String("aa"), 2 },
        { QLatin1String("b"),  3 },
    };

    for (Data *lhs = data; lhs != data + sizeof data / sizeof *data; ++lhs) {
        for (Data *rhs = data; rhs != data + sizeof data / sizeof *data; ++rhs) {
            QLatin1StringContainer l = { lhs->l1 }, r = { rhs->l1 };
            QTest::newRow(qPrintable(QString::asprintf("\"%s\" <> \"%s\"",
                                                       lhs->l1.data() ? lhs->l1.data() : "nullptr",
                                                       rhs->l1.data() ? rhs->l1.data() : "nullptr")))
                << l << lhs->order << r << rhs->order;
        }
    }
}

void tst_QLatin1String::relationalOperators()
{
    QFETCH(QLatin1StringContainer, lhs);
    QFETCH(int, lhsOrderNumber);
    QFETCH(QLatin1StringContainer, rhs);
    QFETCH(int, rhsOrderNumber);

#define CHECK(op) \
    QCOMPARE(lhs.l1 op rhs.l1, lhsOrderNumber op rhsOrderNumber) \
    /*end*/
    CHECK(==);
    CHECK(!=);
    CHECK(< );
    CHECK(> );
    CHECK(<=);
    CHECK(>=);
#undef CHECK
}

QTEST_APPLESS_MAIN(tst_QLatin1String)

#include "tst_qlatin1string.moc"
