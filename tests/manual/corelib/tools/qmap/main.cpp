/****************************************************************************
**
** Copyright (C) 2012 Thorbjørn Lund Martsum - tmartsum[at]gmail.com
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
