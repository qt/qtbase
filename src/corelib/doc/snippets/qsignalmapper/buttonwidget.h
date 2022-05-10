// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BUTTONWIDGET_H
#define BUTTONWIDGET_H

#include <QWidget>

class QSignalMapper;
class QString;

//! [0]
class ButtonWidget : public QWidget
{
    Q_OBJECT

public:
    ButtonWidget(const QStringList &texts, QWidget *parent = nullptr);

signals:
    void clicked(const QString &text);

private:
    QSignalMapper *signalMapper;
//! [0] //! [1]
};
//! [1]

#endif
