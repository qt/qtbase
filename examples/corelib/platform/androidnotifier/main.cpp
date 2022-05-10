// Copyright (C) 2021 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "notificationclient.h"

#include <QApplication>
#include <QWidget>
#include <QPushButton>
#include <QHBoxLayout>
#include <QLabel>
#include <QFont>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QWidget widget;
    QPushButton happyButton;
    happyButton.setIcon(QIcon(":/images/happy.png"));
    happyButton.setIconSize(QSize(happyButton.width(), 120));

    QPushButton sadButton;
    sadButton.setIcon(QIcon(":/images/sad.png"));
    sadButton.setIconSize(QSize(sadButton.width(), 120));

    QVBoxLayout mainLayout;
    QHBoxLayout labelLayout;
    QLabel label = QLabel("Click a smiley to notify your mood");
    QFont font = label.font();
    font.setPointSize(20);
    label.setFont(font);
    labelLayout.addWidget(&label);
    labelLayout.setAlignment(Qt::AlignHCenter);
    mainLayout.addLayout(&labelLayout);

    QHBoxLayout smileysLayout;
    smileysLayout.addWidget(&sadButton);
    smileysLayout.addWidget(&happyButton);
    smileysLayout.setAlignment(Qt::AlignCenter);
    mainLayout.addLayout(&smileysLayout);
    widget.setLayout(&mainLayout);

//! [Connect button signals]
    QObject::connect(&happyButton, &QPushButton::clicked, []() {
        NotificationClient().setNotification("The user is happy!");
    });

    QObject::connect(&sadButton, &QPushButton::clicked, []() {
        NotificationClient().setNotification("The user is sad!");
    });
//! [Connect button signals]

    widget.show();
    return a.exec();
}

