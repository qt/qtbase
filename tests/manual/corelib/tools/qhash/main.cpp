/****************************************************************************
**
** Copyright (C) 2013 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
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
