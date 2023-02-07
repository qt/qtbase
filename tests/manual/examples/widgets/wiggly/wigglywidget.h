// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WIGGLYWIDGET_H
#define WIGGLYWIDGET_H

#include <QBasicTimer>
#include <QWidget>

//! [0]
class WigglyWidget : public QWidget
{
    Q_OBJECT

public:
    WigglyWidget(QWidget *parent = nullptr);

public slots:
    void setText(const QString &newText) { text = newText; }

protected:
    void paintEvent(QPaintEvent *event) override;
    void timerEvent(QTimerEvent *event) override;

private:
    QBasicTimer timer;
    QString text;
    int step;
};
//! [0]

#endif
