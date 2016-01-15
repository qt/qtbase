/****************************************************************************
**
** Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
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
