/****************************************************************************
**
** Copyright (C) 2019 The Qt Company Ltd.
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

#include <QTest>

#include <qaction.h>
#include <qactiongroup.h>

class tst_QActionGroup : public QObject
{
    Q_OBJECT

private slots:
    void cleanup() { QVERIFY(QGuiApplication::topLevelWindows().isEmpty()); }
    void enabledPropagation();
    void visiblePropagation();
    void exclusive();
    void exclusiveOptional();
    void testActionInTwoQActionGroup();
    void unCheckCurrentAction();
};

void tst_QActionGroup::enabledPropagation()
{
    QActionGroup testActionGroup(nullptr);

    auto childAction = new QAction( &testActionGroup );
    auto anotherChildAction = new QAction( &testActionGroup );
    auto freeAction = new QAction(nullptr);

    QVERIFY( testActionGroup.isEnabled() );
    QVERIFY( childAction->isEnabled() );

    testActionGroup.setEnabled( false );
    QVERIFY( !testActionGroup.isEnabled() );
    QVERIFY( !childAction->isEnabled() );
    QVERIFY( !anotherChildAction->isEnabled() );

    childAction->setEnabled(true);
    QVERIFY( !childAction->isEnabled());

    anotherChildAction->setEnabled( false );

    testActionGroup.setEnabled( true );
    QVERIFY( testActionGroup.isEnabled() );
    QVERIFY( childAction->isEnabled() );
    QVERIFY( !anotherChildAction->isEnabled() );

    testActionGroup.setEnabled( false );
    auto lastChildAction = new QAction(&testActionGroup);

    QVERIFY(!lastChildAction->isEnabled());
    testActionGroup.setEnabled( true );
    QVERIFY(lastChildAction->isEnabled());

    freeAction->setEnabled(false);
    testActionGroup.addAction(freeAction);
    QVERIFY(!freeAction->isEnabled());
    delete freeAction;
}

void tst_QActionGroup::visiblePropagation()
{
    QActionGroup testActionGroup(nullptr);

    auto childAction = new QAction( &testActionGroup );
    auto anotherChildAction = new QAction( &testActionGroup );
    auto freeAction = new QAction(nullptr);

    QVERIFY( testActionGroup.isVisible() );
    QVERIFY( childAction->isVisible() );

    testActionGroup.setVisible( false );
    QVERIFY( !testActionGroup.isVisible() );
    QVERIFY( !childAction->isVisible() );
    QVERIFY( !anotherChildAction->isVisible() );

    childAction->setVisible(true);
    QVERIFY( !childAction->isVisible() );

    anotherChildAction->setVisible(false);

    testActionGroup.setVisible( true );
    QVERIFY( testActionGroup.isVisible() );
    QVERIFY( childAction->isVisible() );

    QVERIFY( !anotherChildAction->isVisible() );

    testActionGroup.setVisible( false );
    auto lastChildAction = new QAction(&testActionGroup);

    QVERIFY(!lastChildAction->isVisible());
    testActionGroup.setVisible( true );
    QVERIFY(lastChildAction->isVisible());

    freeAction->setVisible(false);
    testActionGroup.addAction(freeAction);
    QVERIFY(!freeAction->isVisible());
    delete freeAction;
}

void tst_QActionGroup::exclusive()
{
    QActionGroup group(nullptr);
    group.setExclusive(false);
    QVERIFY( !group.isExclusive() );

    auto actOne = new QAction(&group);
    actOne->setCheckable( true );
    auto actTwo = new QAction(&group);
    actTwo->setCheckable( true );
    auto actThree = new QAction(&group);
    actThree->setCheckable( true );

    group.setExclusive( true );
    QVERIFY( !actOne->isChecked() );
    QVERIFY( !actTwo->isChecked() );
    QVERIFY( !actThree->isChecked() );

    actOne->setChecked( true );
    QVERIFY( actOne->isChecked() );
    QVERIFY( !actTwo->isChecked() );
    QVERIFY( !actThree->isChecked() );

    actTwo->setChecked( true );
    QVERIFY( !actOne->isChecked() );
    QVERIFY( actTwo->isChecked() );
    QVERIFY( !actThree->isChecked() );
}

void tst_QActionGroup::exclusiveOptional()
{
    QActionGroup group(0);
    group.setExclusive(true);
    QVERIFY( group.isExclusive() );

    auto actOne = new QAction(&group);
    actOne->setCheckable( true );
    auto actTwo = new QAction(&group);
    actTwo->setCheckable( true );
    auto actThree = new QAction(&group);
    actThree->setCheckable( true );

    QVERIFY( !actOne->isChecked() );
    QVERIFY( !actTwo->isChecked() );
    QVERIFY( !actThree->isChecked() );

    actOne->trigger();
    QVERIFY( actOne->isChecked() );
    QVERIFY( !actTwo->isChecked() );
    QVERIFY( !actThree->isChecked() );

    actOne->trigger();
    QVERIFY( actOne->isChecked() );
    QVERIFY( !actTwo->isChecked() );
    QVERIFY( !actThree->isChecked() );

    group.setExclusionPolicy(QActionGroup::ExclusionPolicy::ExclusiveOptional);
    QVERIFY( group.isExclusive() );

    actOne->trigger();
    QVERIFY( !actOne->isChecked() );
    QVERIFY( !actTwo->isChecked() );
    QVERIFY( !actThree->isChecked() );

    actTwo->trigger();
    QVERIFY( !actOne->isChecked() );
    QVERIFY( actTwo->isChecked() );
    QVERIFY( !actThree->isChecked() );

    actTwo->trigger();
    QVERIFY( !actOne->isChecked() );
    QVERIFY( !actTwo->isChecked() );
    QVERIFY( !actThree->isChecked() );
}

void tst_QActionGroup::testActionInTwoQActionGroup()
{
    QAction action1("Action 1", this);

    QActionGroup group1(this);
    QActionGroup group2(this);

    group1.addAction(&action1);
    group2.addAction(&action1);

    QCOMPARE(action1.actionGroup(), &group2);
    QCOMPARE(group2.actions().constFirst(), &action1);
    QCOMPARE(group1.actions().isEmpty(), true);
}

void tst_QActionGroup::unCheckCurrentAction()
{
    QActionGroup group(nullptr);
    QAction action1(&group) ,action2(&group);
    action1.setCheckable(true);
    action2.setCheckable(true);
    QVERIFY(!action1.isChecked());
    QVERIFY(!action2.isChecked());
    action1.setChecked(true);
    QVERIFY(action1.isChecked());
    QVERIFY(!action2.isChecked());
    auto current = group.checkedAction();
    QCOMPARE(current, &action1);
    current->setChecked(false);
    QVERIFY(!action1.isChecked());
    QVERIFY(!action2.isChecked());
    QVERIFY(!group.checkedAction());
}


QTEST_MAIN(tst_QActionGroup)
#include "tst_qactiongroup.moc"
