// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef COLORLISTEDITOR_H
#define COLORLISTEDITOR_H

#include <QComboBox>

QT_BEGIN_NAMESPACE
class QColor;
class QWidget;
QT_END_NAMESPACE

//! [0]
class ColorListEditor : public QComboBox
{
    Q_OBJECT
    Q_PROPERTY(QColor color READ color WRITE setColor USER true)

public:
    ColorListEditor(QWidget *widget = nullptr);

public:
    QColor color() const;
    void setColor(const QColor &color);

private:
    void populateList();
};
//! [0]

#endif
