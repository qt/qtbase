// Copyright (C) 2013 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only
#include <QDebug>
#include <QList>

void testErase()
{
    QList<int> a, b;
    a.append(5);
    a.append(6);
    a.append(7);
    b = a;
    a.erase(a.begin());
    qDebug() << "erase - Executing line with error now.";
    // a.erase(a.end());
    a.erase(b.begin());
}
void testInsert()
{
    QList<int> a, b;
    a.insert(a.begin(), 1);
    a.insert(a.begin(), 2);
    a.insert(a.end(), 3);
    b = a;
    qDebug() << "insert - Executing line with error now.";
    a.insert(b.begin(), 4);
}
int main()
{
    // testErase();
    testInsert();
    return 0;
}
