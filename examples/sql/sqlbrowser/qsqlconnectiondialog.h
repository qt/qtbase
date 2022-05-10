// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef QSQLCONNECTIONDIALOG_H
#define QSQLCONNECTIONDIALOG_H

#include <QDialog>
#include <QMessageBox>

#include "ui_qsqlconnectiondialog.h"

class QSqlConnectionDialog: public QDialog
{
    Q_OBJECT
public:
    QSqlConnectionDialog(QWidget *parent = nullptr);
    ~QSqlConnectionDialog();

    QString driverName() const;
    QString databaseName() const;
    QString userName() const;
    QString password() const;
    QString hostName() const;
    int port() const;
    bool useInMemoryDatabase() const;

private slots:
    void on_okButton_clicked();
    void on_cancelButton_clicked() { reject(); }
    void on_dbCheckBox_clicked() { ui.connGroupBox->setEnabled(!ui.dbCheckBox->isChecked()); }

private:
    Ui::QSqlConnectionDialogUi ui;
};

#endif
