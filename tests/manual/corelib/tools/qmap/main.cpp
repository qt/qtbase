// Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

//#define Q_NO_DEBUGMAP_PARENT_TEST
// Comment in line above to skip the parent test.
#include <QtCore/QMap>
#include <QDebug>

void noBugErase()
{
    QMap<int, int> a, b;
    a[10] = 11;
    a[11] = 12;
    b = a;
    b.erase(b.begin());
}

void noBugInsertWithHints()
{
    QMap<int, int> a;
    QMap<int, int> b;
    for (int u = 100; u < 10000; u += 20)
        a.insert(u, u);
    b = a;
    QMap<int, int>::const_iterator b_ite(b.begin()); // b.begin() ensures correct detach()
    ++b_ite;
    b.insert(b_ite, 501, 501);
    b.insert(b_ite, 115, 115);
    QMap<int, int> c;
    c = b;
    c.setSharable(false);
}

void testInsertWithHintCorruption()
{
    qDebug() << "Starting testInsertWithHintCorruption";

    QMap<int, int> a;
    QMap<int, int> b;
    for (int u = 100; u < 10000; u += 20)
        a.insert(u, u);
    b = a;
    QMap<int, int>::const_iterator b_ite = b.constBegin();
    ++b_ite;
    b.insert(b_ite, 501, 501);
    b.insert(b_ite, 115, 115); // insert with wrong hint.
    QMap<int, int> c;
    c = b;
    c.setSharable(false);
    qDebug() << "End of testInsertWithHintCorruption - failed silently";
}

void testEraseCorruption()
{
    qDebug() << "Starting testEraseCorruption";

    QMap<int, int> a, b;
    a[10] = 11;
    a[11] = 12;
    b = a;
    b.erase(a.begin());
    qDebug() << "End of testEraseCorruption - failed silently";
}

int main()
{
    noBugErase();
    noBugInsertWithHints();

    // testEraseCorruption();
    testInsertWithHintCorruption();
    return 0;
}
