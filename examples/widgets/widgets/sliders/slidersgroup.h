// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef SLIDERSGROUP_H
#define SLIDERSGROUP_H

#include <QGroupBox>

QT_BEGIN_NAMESPACE
class QDial;
class QScrollBar;
class QSlider;
class QBoxLayout;
QT_END_NAMESPACE

//! [0]
class SlidersGroup : public QGroupBox
{
    Q_OBJECT

public:
    SlidersGroup(const QString &title, QWidget *parent = nullptr);

signals:
    void valueChanged(int value);

public slots:
    void setValue(int value);
    void setMinimum(int value);
    void setMaximum(int value);
    void invertAppearance(bool invert);
    void invertKeyBindings(bool invert);
    void setOrientation(Qt::Orientation orientation);

private:
    QSlider *slider;
    QScrollBar *scrollBar;
    QDial *dial;
    QBoxLayout *slidersLayout;
};
//! [0]

#endif
