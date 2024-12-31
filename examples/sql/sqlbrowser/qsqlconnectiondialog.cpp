// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "qsqlconnectiondialog.h"
#include <ui_qsqlconnectiondialog.h>

#include <QCheckBox>
#include <QMessageBox>
#include <QPushButton>
#include <QSqlDatabase>

QSqlConnectionDialog::QSqlConnectionDialog(QWidget *parent)
    : QDialog(parent)
    , m_ui{std::make_unique<Ui::QSqlConnectionDialogUi>()}
{
    m_ui->setupUi(this);

    QStringList drivers = QSqlDatabase::drivers();

    if (!drivers.contains("QSQLITE"))
        m_ui->dbCheckBox->setEnabled(false);

    m_ui->comboDriver->addItems(drivers);
}

QSqlConnectionDialog::~QSqlConnectionDialog()
    = default;

QString QSqlConnectionDialog::driverName() const
{
    return m_ui->comboDriver->currentText();
}

QString QSqlConnectionDialog::databaseName() const
{
    return m_ui->editDatabase->text();
}

QString QSqlConnectionDialog::userName() const
{
    return m_ui->editUsername->text();
}

QString QSqlConnectionDialog::password() const
{
    return m_ui->editPassword->text();
}

QString QSqlConnectionDialog::hostName() const
{
    return m_ui->editHostname->text();
}

int QSqlConnectionDialog::port() const
{
    return m_ui->portSpinBox->value();
}

bool QSqlConnectionDialog::useInMemoryDatabase() const
{
    return m_ui->dbCheckBox->isChecked();
}

void QSqlConnectionDialog::accept()
{
    if (m_ui->comboDriver->currentText().isEmpty()) {
        QMessageBox::information(this, tr("No database driver selected"),
                                 tr("Please select a database driver"));
        m_ui->comboDriver->setFocus();
    } else {
        QDialog::accept();
    }
}
