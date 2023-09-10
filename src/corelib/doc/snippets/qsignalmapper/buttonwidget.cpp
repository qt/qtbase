// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "buttonwidget.h"

#include <QtWidgets>

//! [0]
ButtonWidget::ButtonWidget(const QStringList &texts, QWidget *parent)
    : QWidget(parent)
{
    signalMapper = new QSignalMapper(this);

    QGridLayout *gridLayout = new QGridLayout(this);
    for (int i = 0; i < texts.size(); ++i) {
        QPushButton *button = new QPushButton(texts[i]);
        connect(button, &QPushButton::clicked, signalMapper, qOverload<>(&QSignalMapper::map));
//! [0] //! [1]
        signalMapper->setMapping(button, texts[i]);
        gridLayout->addWidget(button, i / 3, i % 3);
    }

    connect(signalMapper, &QSignalMapper::mappedString,
//! [1] //! [2]
            this, &ButtonWidget::clicked);
}
//! [2]

//! [3]
ButtonWidget::ButtonWidget(const QStringList &texts, QWidget *parent)
    : QWidget(parent)
{
    QGridLayout *gridLayout = new QGridLayout(this);
    for (int i = 0; i < texts.size(); ++i) {
        QString text = texts[i];
        QPushButton *button = new QPushButton(text);
        connect(button, &QPushButton::clicked, [this, text] { clicked(text); });
        gridLayout->addWidget(button, i / 3, i % 3);
    }
}
//! [3]
