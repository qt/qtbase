// Copyright (C) 2013 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QDebug>
#include <QHash>

void testEraseNoError()
{
    QHash<int, int> a;

    a.insert(100, 100);
    a.insert(101, 200);
    a.insert(5, 50);
    a.insertMulti(5, 60);
    a.insertMulti(5, 70);
    a.insertMulti(5, 80);

    QHash<int, int>::iterator i = a.begin();
    while (i.key() != 5)
        ++i;
    ++i;
    a.erase(i);

    qDebug() << "Erase - got no errors on iterator check";
}

void testErase()
{
    QHash<int, int> a, b;
    a.insert(5, 50);
    a.insert(6, 60);
    a.insert(7, 70);
    b = a;
    a.erase(a.begin());
    b.erase(b.end() - 1);
    qDebug() << "Erase - Executing line with error now.";
    a.erase(b.begin());
}


int main()
{
    testEraseNoError();
    testErase();
    return 0;
}
