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
    , m_ui(new Ui::QSqlConnectionDialogUi)
{
    m_ui->setupUi(this);

    QStringList drivers = QSqlDatabase::drivers();

    if (!drivers.contains("QSQLITE"))
        m_ui->dbCheckBox->setEnabled(false);

    m_ui->comboDriver->addItems(drivers);

    connect(m_ui->okButton, &QPushButton::clicked,
            this, &QSqlConnectionDialog::onOkButton);
    connect(m_ui->cancelButton, &QPushButton::clicked,
            this, &QSqlConnectionDialog::reject);
    connect(m_ui->dbCheckBox, &QCheckBox::stateChanged,
            this, &QSqlConnectionDialog::onDbCheckBox);
}

QSqlConnectionDialog::~QSqlConnectionDialog()
{
    delete m_ui;
}

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

void QSqlConnectionDialog::onOkButton()
{
    if (m_ui->comboDriver->currentText().isEmpty()) {
        QMessageBox::information(this, tr("No database driver selected"),
                                 tr("Please select a database driver"));
        m_ui->comboDriver->setFocus();
    } else {
        accept();
    }
}

void QSqlConnectionDialog::onDbCheckBox()
{
    m_ui->connGroupBox->setEnabled(!m_ui->dbCheckBox->isChecked());
}
