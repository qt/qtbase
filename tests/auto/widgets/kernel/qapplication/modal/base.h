// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef BASE_H
#define BASE_H

#include <QWidget>
#include <QTimer>
#include <QDialog>

class base : public QWidget
{
Q_OBJECT
    QTimer *m_timer;
    bool m_modalStarted = false;
    QDialog *m_modalDialog = nullptr;
public:
    explicit base(QWidget *parent = nullptr);

public slots:
    void periodicTimer();
};

#endif // BASE_H
