/****************************************************************************
**
** Copyright (C) 2013 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QDebug>
//#define QT_STRICT_ITERATORS
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
