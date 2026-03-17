// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef NEWADDRESSTAB_H
#define NEWADDRESSTAB_H

#include <QWidget>

//! [0]
class NewAddressTab : public QWidget
{
    Q_OBJECT

public:
    explicit NewAddressTab(QWidget *parent = nullptr);

signals:
    void triggered();
};
//! [0]

#endif
