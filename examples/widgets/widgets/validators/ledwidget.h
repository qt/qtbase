// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef LEDWIDGET_H
#define LEDWIDGET_H

#include <QLabel>
#include <QPixmap>
#include <QTimer>

class LEDWidget : public QLabel
{
    Q_OBJECT
public:
    LEDWidget(QWidget *parent = nullptr);
public slots:
    void flash();

private slots:
    void extinguish();

private:
    QPixmap onPixmap, offPixmap;
    QTimer flashTimer;
};

#endif // LEDWIDGET_H
