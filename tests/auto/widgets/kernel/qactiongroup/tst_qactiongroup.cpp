/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include <qmainwindow.h>
#include <qmenu.h>
#include <qaction.h>
#include <qactiongroup.h>

class tst_QActionGroup : public QObject
{
    Q_OBJECT

private slots:
    void cleanup() { QVERIFY(QApplication::topLevelWidgets().isEmpty()); }
    void separators();
};

void tst_QActionGroup::separators()
{
    QMainWindow mw;
    QMenu menu(&mw);
    QActionGroup actGroup(&mw);

    mw.show();

    QAction *action = new QAction(&actGroup);
    action->setText("test one");

    QAction *separator = new QAction(&actGroup);
    separator->setSeparator(true);
    actGroup.addAction(separator);

    menu.addActions(actGroup.actions());

    QCOMPARE(menu.actions().size(), 2);

    const auto removeActions = [&menu](const QList<QAction *> &actions) {
        for (QAction *action : actions)
            menu.removeAction(action);
    };
    removeActions(actGroup.actions());

    QCOMPARE(menu.actions().size(), 0);

    action = new QAction(&actGroup);
    action->setText("test two");

    menu.addActions(actGroup.actions());

    QCOMPARE(menu.actions().size(), 3);
}

QTEST_MAIN(tst_QActionGroup)
#include "tst_qactiongroup.moc"
