// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>
#include <QGridLayout>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QComboBox;
class QGroupBox;
class QLabel;
class QSpinBox;
class QStackedWidget;
QT_END_NAMESPACE
class SlidersGroup;

//! [0]
class Window : public QWidget
{
    Q_OBJECT

public:
    Window(QWidget *parent = nullptr);

private:
    void createControls(const QString &title);
    void resizeEvent(QResizeEvent *e);

    SlidersGroup *slidersGroup;

    QGroupBox *controlsGroup;
    QLabel *minimumLabel;
    QLabel *maximumLabel;
    QLabel *valueLabel;
    QCheckBox *invertedAppearance;
    QCheckBox *invertedKeyBindings;
    QSpinBox *minimumSpinBox;
    QSpinBox *maximumSpinBox;
    QSpinBox *valueSpinBox;
    QBoxLayout *layout;
};
//! [0]

#endif
