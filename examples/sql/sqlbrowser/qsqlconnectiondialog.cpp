/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qsqlconnectiondialog.h"
#include "ui_qsqlconnectiondialog.h"

#include <QSqlDatabase>

QSqlConnectionDialog::QSqlConnectionDialog(QWidget *parent)
    : QDialog(parent)
{
    ui.setupUi(this);

    QStringList drivers = QSqlDatabase::drivers();

    // remove compat names
    drivers.removeAll("QMYSQL3");
    drivers.removeAll("QOCI8");
    drivers.removeAll("QODBC3");
    drivers.removeAll("QPSQL7");
    drivers.removeAll("QTDS7");

    if (!drivers.contains("QSQLITE"))
        ui.dbCheckBox->setEnabled(false);

    ui.comboDriver->addItems(drivers);
}

QSqlConnectionDialog::~QSqlConnectionDialog()
{
}

QString QSqlConnectionDialog::driverName() const
{
    return ui.comboDriver->currentText();
}

QString QSqlConnectionDialog::databaseName() const
{
    return ui.editDatabase->text();
}

QString QSqlConnectionDialog::userName() const
{
    return ui.editUsername->text();
}

QString QSqlConnectionDialog::password() const
{
    return ui.editPassword->text();
}

QString QSqlConnectionDialog::hostName() const
{
    return ui.editHostname->text();
}

int QSqlConnectionDialog::port() const
{
    return ui.portSpinBox->value();
}

bool QSqlConnectionDialog::useInMemoryDatabase() const
{
    return ui.dbCheckBox->isChecked();
}

void QSqlConnectionDialog::on_okButton_clicked()
{
    if (ui.comboDriver->currentText().isEmpty()) {
        QMessageBox::information(this, tr("No database driver selected"),
                                 tr("Please select a database driver"));
        ui.comboDriver->setFocus();
    } else {
        accept();
    }
}
