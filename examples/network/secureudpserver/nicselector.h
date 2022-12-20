// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef NICSELECTOR_H
#define NICSELECTOR_H

#include <QDialog>
#include <QHostAddress>
#include <QList>

QT_BEGIN_NAMESPACE
namespace Ui {
class NicSelector;
}
QT_END_NAMESPACE

class NicSelector : public QDialog
{
    Q_OBJECT

public:
    explicit NicSelector(QWidget *parent = nullptr);
    ~NicSelector();

    QHostAddress selectedIp() const;
    quint16 selectedPort() const;

private:
    Ui::NicSelector *ui = nullptr;
    QList<QHostAddress> availableAddresses;
};

#endif // NICSELECTOR_H
