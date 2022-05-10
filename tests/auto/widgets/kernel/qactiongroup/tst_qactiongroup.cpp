// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

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
