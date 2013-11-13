/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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

#include <QtTest/QtTest>

#include <qlocale.h>
#include <qcollator.h>

#include <cstring>

class tst_QCollator : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void moveSemantics();
};

#ifdef Q_COMPILER_RVALUE_REFS
static bool dpointer_is_null(QCollator &c)
{
    char mem[sizeof c];
    using namespace std;
    memcpy(mem, &c, sizeof c);
    for (size_t i = 0; i < sizeof c; ++i)
        if (mem[i])
            return false;
    return true;
}
#endif

void tst_QCollator::moveSemantics()
{
#ifdef Q_COMPILER_RVALUE_REFS
    const QLocale de_AT(QLocale::German, QLocale::Austria);

    QCollator c1(de_AT);
    QCOMPARE(c1.locale(), de_AT);

    QCollator c2(std::move(c1));
    QCOMPARE(c2.locale(), de_AT);
    QVERIFY(dpointer_is_null(c1));

    c1 = std::move(c2);
    QCOMPARE(c1.locale(), de_AT);
    QVERIFY(dpointer_is_null(c2));
#else
    QSKIP("The compiler is not in C++11 mode or does not support move semantics.");
#endif
}

QTEST_APPLESS_MAIN(tst_QCollator)

#include "tst_qcollator.moc"
