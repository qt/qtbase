// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include <QApplication>
#include <QFont>
#include <QHBoxLayout>
#include <QJniObject>
#include <QtJniTypes>
#include <QLabel>
#include <QPushButton>
#include <QWidget>

#include "dataexchanger.h"
#include "otherlanguagehandler.h"

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    QWidget widget;
    QLabel label = QLabel("Click a button to send data\nto the other language");
    QFont font = label.font();
    font.setPointSize(20);
    label.setFont(font);

    QLabel fromOtherLabel;
    QFont fromOtherFont = fromOtherLabel.font();
    fromOtherFont.setPointSize(16);
    fromOtherLabel.setFont(fromOtherFont);

    QPushButton javaButton;
    javaButton.setText("Hello Java!");
    javaButton.setFont(font);
    javaButton.setFixedSize(QSize(150, 50));

    QPushButton kotlinButton;
    kotlinButton.setText("Hello Kotlin!");
    kotlinButton.setFont(font);
    kotlinButton.setFixedSize(QSize(150, 50));

    QVBoxLayout mainLayout;
    QVBoxLayout labelLayout;

    labelLayout.addWidget(&label);
    labelLayout.addWidget(&fromOtherLabel);
    labelLayout.setAlignment(Qt::AlignHCenter);
    mainLayout.addLayout(&labelLayout);

    QHBoxLayout buttonsLayout;
    buttonsLayout.addWidget(&kotlinButton);
    buttonsLayout.addWidget(&javaButton);
    buttonsLayout.setAlignment(Qt::AlignCenter);
    mainLayout.addLayout(&buttonsLayout);
    widget.setLayout(&mainLayout);

    DataExchanger dataExchanger;

//! [Connect button signals]
    QObject::connect(&javaButton, &QPushButton::clicked, &javaButton, [&]() {
        dataExchanger.fromCpp(DataExchanger::Java, "Hello!");
    });

    QObject::connect(&kotlinButton, &QPushButton::clicked, &kotlinButton, [&]() {
        dataExchanger.fromCpp(DataExchanger::Kotlin, "Hello!");
    });
    QObject::connect(&dataExchanger, &DataExchanger::fromOther,
                     &dataExchanger, [&](const QString& str) {
        fromOtherLabel.setText(str);
    });
//! [Connect button signals]

    OtherLanguageHandler otherHandler(&dataExchanger);
    widget.show();
    return a.exec();
}

