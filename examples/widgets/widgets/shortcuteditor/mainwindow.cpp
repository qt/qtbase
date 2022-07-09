// Copyright (C) 2022 Laszlo Papp <lpapp@kde.org>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "mainwindow.h"

#include "actionmanager.h"
#include "application.h"
#include "shortcuteditorwidget.h"

#include <QAction>
#include <QHBoxLayout>
#include <QPushButton>
#include <QVBoxLayout>

MainWindow::MainWindow()
{
    QPushButton *topPushButton = new QPushButton("Left");
    QPushButton *bottomPushButton = new QPushButton("Right");
    for (auto nameShortcut : std::vector<std::vector<const char *>>{{"red", "r", "shift+r"}, {"green", "g", "shift+g"}, {"blue", "b", "shift+b"}}) {
        Application *application = static_cast<Application *>(QCoreApplication::instance());
        ActionManager *actionManager = application->actionManager();
        QAction *action = actionManager->registerAction(nameShortcut[0], nameShortcut[1], "top", "color");
        topPushButton->addAction(action);
        connect(action, &QAction::triggered, this, [topPushButton, nameShortcut]() {
            topPushButton->setText(nameShortcut[0]);
        });

        action = actionManager->registerAction(nameShortcut[0], nameShortcut[2], "bottom", "color");
        bottomPushButton->addAction(action);
        connect(action, &QAction::triggered, this, [bottomPushButton, nameShortcut]() {
            bottomPushButton->setText(nameShortcut[0]);
        });
    }

    QVBoxLayout *vBoxLayout = new QVBoxLayout;
    vBoxLayout->addWidget(topPushButton);
    vBoxLayout->addWidget(bottomPushButton);

    QHBoxLayout *hBoxLayout = new QHBoxLayout;
    hBoxLayout->addWidget(new ShortcutEditorWidget);
    hBoxLayout->addLayout(vBoxLayout);

    QWidget *centralWidget = new QWidget;
    centralWidget->setLayout(hBoxLayout);
    setCentralWidget(centralWidget);
}
