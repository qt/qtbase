// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef TESTWIDGET_H
#define TESTWIDGET_H

#include <QSlider>
#include <QStringList>
#include <QWidget>

class ElidedLabel;

//! [0]
class TestWidget : public QWidget
{
    Q_OBJECT

public:
    TestWidget(QWidget *parent = nullptr);

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void switchText();
    void onWidthChanged(int width);
    void onHeightChanged(int height);

private:
    int sampleIndex;
    QStringList textSamples;
    ElidedLabel *elidedText;
    QSlider *heightSlider;
    QSlider *widthSlider;
};
//! [0]

#endif // TESTWIDGET_H
