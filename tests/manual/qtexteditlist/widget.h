// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>
#include <QTextCursor>

namespace Ui {
class Widget;
}

class Widget : public QWidget
{
    Q_OBJECT

public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget();
public slots:
    void setFontPointSize(int value);
    void setIndentWidth(int value);
private:
    Ui::Widget *ui;
    QTextCursor* textCursor;
};

#endif // WIDGET_H
