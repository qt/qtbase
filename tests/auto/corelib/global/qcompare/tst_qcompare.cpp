// Copyright (C) 2020 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Giuseppe D'Angelo <giuseppe.dangelo@kdab.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QtCore/QtCompare>
#include <QtTest/QTest>

class tst_QCompare: public QObject
{
    Q_OBJECT
private slots:
    void partialOrdering();
};

void tst_QCompare::partialOrdering()
{
    static_assert(QPartialOrdering::Unordered == QPartialOrdering::Unordered);
    static_assert(QPartialOrdering::Unordered != QPartialOrdering::Less);
    static_assert(QPartialOrdering::Unordered != QPartialOrdering::Equivalent);
    static_assert(QPartialOrdering::Unordered != QPartialOrdering::Greater);

    static_assert(QPartialOrdering::Less != QPartialOrdering::Unordered);
    static_assert(QPartialOrdering::Less == QPartialOrdering::Less);
    static_assert(QPartialOrdering::Less != QPartialOrdering::Equivalent);
    static_assert(QPartialOrdering::Less != QPartialOrdering::Greater);

    static_assert(QPartialOrdering::Equivalent != QPartialOrdering::Unordered);
    static_assert(QPartialOrdering::Equivalent != QPartialOrdering::Less);
    static_assert(QPartialOrdering::Equivalent == QPartialOrdering::Equivalent);
    static_assert(QPartialOrdering::Equivalent != QPartialOrdering::Greater);

    static_assert(QPartialOrdering::Greater != QPartialOrdering::Unordered);
    static_assert(QPartialOrdering::Greater != QPartialOrdering::Less);
    static_assert(QPartialOrdering::Greater != QPartialOrdering::Equivalent);
    static_assert(QPartialOrdering::Greater == QPartialOrdering::Greater);


    static_assert(!(QPartialOrdering::Unordered == 0));
    static_assert(!(QPartialOrdering::Unordered != 0));
    static_assert(!(QPartialOrdering::Unordered <  0));
    static_assert(!(QPartialOrdering::Unordered <= 0));
    static_assert(!(QPartialOrdering::Unordered >  0));
    static_assert(!(QPartialOrdering::Unordered >= 0));

    static_assert(!(0 == QPartialOrdering::Unordered));
    static_assert(!(0 != QPartialOrdering::Unordered));
    static_assert(!(0 <  QPartialOrdering::Unordered));
    static_assert(!(0 <= QPartialOrdering::Unordered));
    static_assert(!(0 >  QPartialOrdering::Unordered));
    static_assert(!(0 >= QPartialOrdering::Unordered));


    static_assert(!(QPartialOrdering::Less == 0));
    static_assert( (QPartialOrdering::Less != 0));
    static_assert( (QPartialOrdering::Less <  0));
    static_assert( (QPartialOrdering::Less <= 0));
    static_assert(!(QPartialOrdering::Less >  0));
    static_assert(!(QPartialOrdering::Less >= 0));

    static_assert(!(0 == QPartialOrdering::Less));
    static_assert( (0 != QPartialOrdering::Less));
    static_assert(!(0 <  QPartialOrdering::Less));
    static_assert(!(0 <= QPartialOrdering::Less));
    static_assert( (0 >  QPartialOrdering::Less));
    static_assert( (0 >= QPartialOrdering::Less));


    static_assert( (QPartialOrdering::Equivalent == 0));
    static_assert(!(QPartialOrdering::Equivalent != 0));
    static_assert(!(QPartialOrdering::Equivalent <  0));
    static_assert( (QPartialOrdering::Equivalent <= 0));
    static_assert(!(QPartialOrdering::Equivalent >  0));
    static_assert( (QPartialOrdering::Equivalent >= 0));

    static_assert( (0 == QPartialOrdering::Equivalent));
    static_assert(!(0 != QPartialOrdering::Equivalent));
    static_assert(!(0 <  QPartialOrdering::Equivalent));
    static_assert( (0 <= QPartialOrdering::Equivalent));
    static_assert(!(0 >  QPartialOrdering::Equivalent));
    static_assert( (0 >= QPartialOrdering::Equivalent));


    static_assert(!(QPartialOrdering::Greater == 0));
    static_assert( (QPartialOrdering::Greater != 0));
    static_assert(!(QPartialOrdering::Greater <  0));
    static_assert(!(QPartialOrdering::Greater <= 0));
    static_assert( (QPartialOrdering::Greater >  0));
    static_assert( (QPartialOrdering::Greater >= 0));

    static_assert(!(0 == QPartialOrdering::Greater));
    static_assert( (0 != QPartialOrdering::Greater));
    static_assert( (0 <  QPartialOrdering::Greater));
    static_assert( (0 <= QPartialOrdering::Greater));
    static_assert(!(0 >  QPartialOrdering::Greater));
    static_assert(!(0 >= QPartialOrdering::Greater));
}

QTEST_MAIN(tst_QCompare)
#include "tst_qcompare.moc"
