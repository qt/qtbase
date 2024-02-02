// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include "message.h"

QT_FORWARD_DECLARE_CLASS(QTextEdit)

//! [Window class definition]
class Window : public QWidget
{
    Q_OBJECT

public:
    Window(QWidget *parent = nullptr);

signals:
    void messageSent(const Message &message);

public slots:
    void setMessage(const Message &message);

private slots:
    void sendMessage();

private:
    Message thisMessage;
    QTextEdit *editor;
};
//! [Window class definition]

#endif
