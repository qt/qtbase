// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SCREENSHOT_H
#define SCREENSHOT_H

#include <QPixmap>
#include <QWidget>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QGridLayout;
class QGroupBox;
class QHBoxLayout;
class QLabel;
class QPushButton;
class QSpinBox;
class QVBoxLayout;
QT_END_NAMESPACE

//! [0]
class Screenshot : public QWidget
{
    Q_OBJECT

public:
    Screenshot();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void newScreenshot();
    void saveScreenshot();
    void shootScreen();
    void updateCheckBox();

private:
    void updateScreenshotLabel();

    QPixmap originalPixmap;

    QLabel *screenshotLabel;
    QSpinBox *delaySpinBox;
    QCheckBox *hideThisWindowCheckBox;
    QPushButton *newScreenshotButton;
};
//! [0]

#endif // SCREENSHOT_H
