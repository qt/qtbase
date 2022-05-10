// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef NEWADDRESSTAB_H
#define NEWADDRESSTAB_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QLabel;
class QPushButton;
class QVBoxLayout;
QT_END_NAMESPACE

//! [0]
class NewAddressTab : public QWidget
{
    Q_OBJECT

public:
    NewAddressTab(QWidget *parent = nullptr);

public slots:
    void addEntry();

signals:
    void sendDetails(const QString &name, const QString &address);
};
//! [0]

#endif
