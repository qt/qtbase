/****************************************************************************
**
** Copyright (C) 2013 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
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
