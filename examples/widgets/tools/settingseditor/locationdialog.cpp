/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <QtWidgets>

#include "locationdialog.h"

LocationDialog::LocationDialog(QWidget *parent)
    : QDialog(parent)
{
    formatComboBox = new QComboBox;
    formatComboBox->addItem(tr("Native"));
    formatComboBox->addItem(tr("INI"));

    scopeComboBox = new QComboBox;
    scopeComboBox->addItem(tr("User"));
    scopeComboBox->addItem(tr("System"));

    organizationComboBox = new QComboBox;
    organizationComboBox->addItem(tr("QtProject"));
    organizationComboBox->setEditable(true);

    applicationComboBox = new QComboBox;
    applicationComboBox->addItem(tr("Any"));
    applicationComboBox->addItem(tr("Qt Creator"));
    applicationComboBox->addItem(tr("Application Example"));
    applicationComboBox->addItem(tr("Assistant"));
    applicationComboBox->addItem(tr("Designer"));
    applicationComboBox->addItem(tr("Linguist"));
    applicationComboBox->setEditable(true);
    applicationComboBox->setCurrentIndex(1);

    formatLabel = new QLabel(tr("&Format:"));
    formatLabel->setBuddy(formatComboBox);

    scopeLabel = new QLabel(tr("&Scope:"));
    scopeLabel->setBuddy(scopeComboBox);

    organizationLabel = new QLabel(tr("&Organization:"));
    organizationLabel->setBuddy(organizationComboBox);

    applicationLabel = new QLabel(tr("&Application:"));
    applicationLabel->setBuddy(applicationComboBox);

    locationsGroupBox = new QGroupBox(tr("Setting Locations"));

    QStringList labels;
    labels << tr("Location") << tr("Access");

    locationsTable = new QTableWidget;
    locationsTable->setSelectionMode(QAbstractItemView::SingleSelection);
    locationsTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    locationsTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    locationsTable->setColumnCount(2);
    locationsTable->setHorizontalHeaderLabels(labels);
    locationsTable->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    locationsTable->horizontalHeader()->resizeSection(1, 180);
    connect(locationsTable, &QTableWidget::itemActivated, this, &LocationDialog::itemActivated);

    buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel);

    typedef void (QComboBox::*QComboIntSignal)(int);
    connect(formatComboBox, static_cast<QComboIntSignal>(&QComboBox::activated),
            this, &LocationDialog::updateLocationsTable);
    connect(scopeComboBox, static_cast<QComboIntSignal>(&QComboBox::activated),
            this, &LocationDialog::updateLocationsTable);
    connect(organizationComboBox->lineEdit(),
            &QLineEdit::editingFinished,
            this, &LocationDialog::updateLocationsTable);
    connect(applicationComboBox->lineEdit(),
            &QLineEdit::editingFinished,
            this, &LocationDialog::updateLocationsTable);
    connect(applicationComboBox, static_cast<QComboIntSignal>(&QComboBox::activated),
            this, &LocationDialog::updateLocationsTable);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    QVBoxLayout *locationsLayout = new QVBoxLayout(locationsGroupBox);
    locationsLayout->addWidget(locationsTable);

    QGridLayout *mainLayout = new QGridLayout(this);
    mainLayout->addWidget(formatLabel, 0, 0);
    mainLayout->addWidget(formatComboBox, 0, 1);
    mainLayout->addWidget(scopeLabel, 1, 0);
    mainLayout->addWidget(scopeComboBox, 1, 1);
    mainLayout->addWidget(organizationLabel, 2, 0);
    mainLayout->addWidget(organizationComboBox, 2, 1);
    mainLayout->addWidget(applicationLabel, 3, 0);
    mainLayout->addWidget(applicationComboBox, 3, 1);
    mainLayout->addWidget(locationsGroupBox, 4, 0, 1, 2);
    mainLayout->addWidget(buttonBox, 5, 0, 1, 2);

    updateLocationsTable();

    setWindowTitle(tr("Open Application Settings"));
    resize(650, 400);
}

QSettings::Format LocationDialog::format() const
{
    if (formatComboBox->currentIndex() == 0)
        return QSettings::NativeFormat;
    else
        return QSettings::IniFormat;
}

QSettings::Scope LocationDialog::scope() const
{
    if (scopeComboBox->currentIndex() == 0)
        return QSettings::UserScope;
    else
        return QSettings::SystemScope;
}

QString LocationDialog::organization() const
{
    return organizationComboBox->currentText();
}

QString LocationDialog::application() const
{
    if (applicationComboBox->currentText() == tr("Any"))
        return QString();
    else
        return applicationComboBox->currentText();
}

void LocationDialog::itemActivated(QTableWidgetItem *)
{
    buttonBox->button(QDialogButtonBox::Ok)->animateClick();
}

void LocationDialog::updateLocationsTable()
{
    locationsTable->setUpdatesEnabled(false);
    locationsTable->setRowCount(0);

    for (int i = 0; i < 2; ++i) {
        if (i == 0 && scope() == QSettings::SystemScope)
            continue;

        QSettings::Scope actualScope = (i == 0) ? QSettings::UserScope
                                                : QSettings::SystemScope;
        for (int j = 0; j < 2; ++j) {
            if (j == 0 && application().isEmpty())
                continue;

            QString actualApplication;
            if (j == 0)
                actualApplication = application();
            QSettings settings(format(), actualScope, organization(),
                               actualApplication);

            int row = locationsTable->rowCount();
            locationsTable->setRowCount(row + 1);

            QTableWidgetItem *item0 = new QTableWidgetItem(QDir::toNativeSeparators(settings.fileName()));

            QTableWidgetItem *item1 = new QTableWidgetItem;
            bool disable = (settings.childKeys().isEmpty()
                            && settings.childGroups().isEmpty());

            if (row == 0) {
                if (settings.isWritable()) {
                    item1->setText(tr("Read-write"));
                    disable = false;
                } else {
                    item1->setText(tr("Read-only"));
                }
                buttonBox->button(QDialogButtonBox::Ok)->setDisabled(disable);
            } else {
                item1->setText(tr("Read-only fallback"));
            }

            if (disable) {
                item0->setFlags(item0->flags() & ~Qt::ItemIsEnabled);
                item1->setFlags(item1->flags() & ~Qt::ItemIsEnabled);
            }

            locationsTable->setItem(row, 0, item0);
            locationsTable->setItem(row, 1, item1);
        }
    }
    locationsTable->setUpdatesEnabled(true);
}
