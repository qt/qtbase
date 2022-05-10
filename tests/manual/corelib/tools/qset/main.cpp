// Copyright (C) 2013 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#include <QDebug>
#include <QSet>

void testErase()
{
    QSet<int> a, b;
    a.insert(5);
    a.insert(6);
    a.insert(7);
    b = a;
    a.erase(a.begin());
    b.erase(b.end() - 1);
    qDebug() << "erase - no errors until now";
    a.erase(b.begin());
}

int main()
{
    testErase();
    return 0;
}
