// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
#ifndef ADDRESSDIALOG_H
#define ADDRESSDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui {
class AddressDialog;
}
QT_END_NAMESPACE

class AddressDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddressDialog(QWidget *parent = nullptr);
    ~AddressDialog();

    QString remoteName() const;
    quint16 remotePort() const;

private:
    void setupHostSelector();
    void setupPortSelector();

    Ui::AddressDialog *ui = nullptr;
};

#endif // ADDRESSDIALOG_H
