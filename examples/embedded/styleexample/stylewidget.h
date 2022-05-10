// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#ifndef STYLEWIDGET_H
#define STYLEWIDGET_H

#include <QFrame>

#include "ui_stylewidget.h"

class StyleWidget : public QFrame
{
    Q_OBJECT
public:
    StyleWidget(QWidget *parent = nullptr);

private:
    Ui_StyleWidget m_ui;

private slots:
    void on_close_clicked();
    void on_blueStyle_clicked();
    void on_khakiStyle_clicked();
    void on_noStyle_clicked();
    void on_transparentStyle_clicked();
};

#endif
