// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef QSQLCONNECTIONDIALOG_H
#define QSQLCONNECTIONDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui
{
class QSqlConnectionDialogUi;
}
QT_END_NAMESPACE

class QSqlConnectionDialog : public QDialog
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
    void onOkButton();
    void onDbCheckBox();

private:
    Ui::QSqlConnectionDialogUi *m_ui;
};

#endif
