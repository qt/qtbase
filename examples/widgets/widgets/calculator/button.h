// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef BUTTON_H
#define BUTTON_H

#include <QToolButton>

//! [0]
class Button : public QToolButton
{
    Q_OBJECT

public:
    explicit Button(const QString &text, QWidget *parent = nullptr);

    QSize sizeHint() const override;
};
//! [0]

#endif
