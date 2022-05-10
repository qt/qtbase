// Copyright (C) 2016 Alex Trotsenko <alex1973tr@gmail.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CHATCONSUMER_H
#define CHATCONSUMER_H

#include "consumer.h"

QT_BEGIN_NAMESPACE
class QTextEdit;
class QLineEdit;
QT_END_NAMESPACE

class ChatConsumer : public Consumer
{
    Q_OBJECT
public:
    explicit ChatConsumer(QObject *parent = nullptr);

    QWidget *widget() override;
    void readDatagram(const QByteArray &ba) override;

private slots:
    void returnPressed();

private:
    QWidget *frameWidget;
    QTextEdit *textEdit;
    QLineEdit *lineEdit;
};

#endif
