// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "chatconsumer.h"
#include <QWidget>
#include <QTextEdit>
#include <QLineEdit>
#include <QVBoxLayout>
#include <QString>

ChatConsumer::ChatConsumer(QObject *parent)
    : Consumer(parent)
{
    frameWidget = new QWidget;
    frameWidget->setFocusPolicy(Qt::TabFocus);

    textEdit = new QTextEdit;
    textEdit->setFocusPolicy(Qt::NoFocus);
    textEdit->setReadOnly(true);

    lineEdit = new QLineEdit;
    frameWidget->setFocusProxy(lineEdit);

    connect(lineEdit, &QLineEdit::returnPressed, this, &ChatConsumer::returnPressed);

    QVBoxLayout *layout = new QVBoxLayout(frameWidget);
    layout->setContentsMargins( 0, 0, 0, 0);
    layout->addWidget(textEdit);
    layout->addWidget(lineEdit);
}

QWidget *ChatConsumer::widget()
{
    return frameWidget;
}

void ChatConsumer::readDatagram(const QByteArray &ba)
{
    textEdit->append(QString::fromUtf8(ba));
}

void ChatConsumer::returnPressed()
{
    emit writeDatagram(lineEdit->text().toUtf8());
    lineEdit->clear();
}
