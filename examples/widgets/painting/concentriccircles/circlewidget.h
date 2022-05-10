// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef CIRCLEWIDGET_H
#define CIRCLEWIDGET_H

#include <QWidget>

//! [0]
class CircleWidget : public QWidget
{
    Q_OBJECT

public:
    CircleWidget(QWidget *parent = nullptr);

    void setFloatBased(bool floatBased);
    void setAntialiased(bool antialiased);

    QSize minimumSizeHint() const override;
    QSize sizeHint() const override;

public slots:
    void nextAnimationFrame();

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    bool floatBased;
    bool antialiased;
    int frameNo;
};
//! [0]

#endif // CIRCLEWIDGET_H
