/****************************************************************************
**
** Copyright (C) 2018 Klarälvdalens Datakonsult AB, a KDAB Group company, info@kdab.com, author Sérgio Martins <sergio.martins@kdab.com>
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

#include <QtTest/QtTest>
#include <QtCore/QScopeGuard>

/*!
 \class tst_QScopedGuard
 \internal
 \since 5.11
 \brief Tests class QScopedCleanup and function qScopeGuard

 */
class tst_QScopedGuard : public QObject
{
    Q_OBJECT

private Q_SLOTS:
    void leavingScope();
    void exceptions();
};

static int s_globalState = 0;

void tst_QScopedGuard::leavingScope()
{
    auto cleanup = qScopeGuard([] { s_globalState++; QCOMPARE(s_globalState, 3); });
    QCOMPARE(s_globalState, 0);

    {
        auto cleanup = qScopeGuard([] { s_globalState++; });
        QCOMPARE(s_globalState, 0);
    }

    QCOMPARE(s_globalState, 1);
    s_globalState++;
}

void tst_QScopedGuard::exceptions()
{
    s_globalState = 0;
    bool caught = false;
    QT_TRY
    {
        auto cleanup = qScopeGuard([] { s_globalState++; });
        QT_THROW(std::bad_alloc()); //if Qt compiled without exceptions this is noop
        s_globalState = 100;
    }
    QT_CATCH(...)
    {
        caught = true;
        QCOMPARE(s_globalState, 1);
    }

    QVERIFY((caught && s_globalState == 1) || (!caught && s_globalState == 101));

}

QTEST_MAIN(tst_QScopedGuard)
#include "tst_qscopeguard.moc"
