// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "addresswidget.h"
#include "adddialog.h"
#include "newaddresstab.h"
#include "tablemodel.h"

#include <QHeaderView>
#include <QMessageBox>
#include <QTableView>

#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QModelIndex>
#include <QRegularExpression>
#include <QSortFilterProxyModel>
#include <QStandardPaths>

#include <algorithm>

using namespace Qt::StringLiterals;

QString AddressWidget::fileName()
{
    static const QString result = QStandardPaths::standardLocations(QStandardPaths::TempLocation).value(0)
            + "/addressbook.dat"_L1;
    return result;
}

//! [0]
AddressWidget::AddressWidget(QWidget *parent)
    : QTabWidget(parent),
      table(new TableModel(this)),
      newAddressTab(new NewAddressTab(this))
{
    connect(newAddressTab, &NewAddressTab::triggered, this, &AddressWidget::showAddEntryDialog);

    addTab(newAddressTab, tr("Address Book"));

    setupTabs();
}
//! [0]

//! [2]
void AddressWidget::showAddEntryDialog()
{
    AddDialog aDialog(this);

    if (aDialog.exec() == QDialog::Accepted)
        addEntry(aDialog.contact());
}
//! [2]

//! [3]
void AddressWidget::addEntry(const Contact &contact)
{
    if (table->getContacts().contains(contact)) {
        QMessageBox::information(this, tr("Duplicate Name"),
                                 tr("The name \"%1\" already exists.").arg(contact.name));
        return;
    }

    table->insertRows(0, 1);
    table->setData(table->index(0, 0), contact.name, Qt::EditRole);
    table->setData(table->index(0, 1), contact.address, Qt::EditRole);
    removeTab(indexOf(newAddressTab));

    const QChar firstChar = contact.name.at(0).toUpper();
    for (int t = 0, tabCount = count(); t < tabCount; ++t) {
        if (tabText(t).contains(firstChar)) {
            setCurrentIndex(t);
            break;
        }
    }
}
//! [3]

//! [4a]
void AddressWidget::editEntry()
{
    auto *tableView = static_cast<QTableView *>(currentWidget());
    auto *proxy = static_cast<QSortFilterProxyModel *>(tableView->model());
    QItemSelectionModel *selectionModel = tableView->selectionModel();

    const QModelIndexList indexes = selectionModel->selectedRows();
    if (indexes.isEmpty())
        return;

    const int row = proxy->mapToSource(indexes.constFirst()).row();

    Contact contact;
    QModelIndex nameIndex = table->index(row, 0);
    QVariant varName = table->data(nameIndex, Qt::DisplayRole);
    contact.name = varName.toString();
    QModelIndex addressIndex = table->index(row, 1);
    QVariant varAddr = table->data(addressIndex, Qt::DisplayRole);
    contact.address = varAddr.toString();
//! [4a]

//! [4b]
    AddDialog aDialog(this);
    aDialog.setWindowTitle(tr("Edit a Contact"));
    aDialog.editAddress(contact);

    if (aDialog.exec() == QDialog::Accepted) {
        const Contact newContact = aDialog.contact();
        if (newContact != contact) {
            const QModelIndex index = table->index(row, 1);
            table->setData(index, newContact.address, Qt::EditRole);
        }
    }
}
//! [4b]

//! [5]
void AddressWidget::removeEntry()
{
    auto *tableView = static_cast<QTableView *>(currentWidget());
    auto *proxy = static_cast<QSortFilterProxyModel *>(tableView->model());
    QItemSelectionModel *selectionModel = tableView->selectionModel();

    const QModelIndexList indexes = selectionModel->selectedRows();
    if (indexes.isEmpty())
        return;

    const int row = proxy->mapToSource(indexes.constFirst()).row();
    table->removeRows(row, 1);

    if (table->rowCount() == 0)
        insertTab(0, newAddressTab, tr("Address Book"));
}
//! [5]

//! [1]
void AddressWidget::setupTabs()
{
    const auto groups = { "ABC"_L1, "DEF"_L1, "GHI"_L1, "JKL"_L1, "MNO"_L1, "PQR"_L1,
                          "STU"_L1, "VW"_L1, "XYZ"_L1 };

    for (QLatin1StringView str : groups) {
        const auto regExp = QRegularExpression(QLatin1StringView("^[%1].*").arg(str),
                                               QRegularExpression::CaseInsensitiveOption);

        auto *proxyModel = new QSortFilterProxyModel(this);
        proxyModel->setSourceModel(table);
        proxyModel->setFilterRegularExpression(regExp);
        proxyModel->setFilterKeyColumn(0);

        auto *tableView = new QTableView;
        tableView->setModel(proxyModel);
        tableView->setSelectionBehavior(QAbstractItemView::SelectRows);
        tableView->horizontalHeader()->setStretchLastSection(true);
        tableView->verticalHeader()->hide();
        tableView->setEditTriggers(QAbstractItemView::NoEditTriggers);
        tableView->setSelectionMode(QAbstractItemView::SingleSelection);
        tableView->setSortingEnabled(true);

        connect(tableView->selectionModel(), &QItemSelectionModel::selectionChanged,
                this, &AddressWidget::selectionChanged);

        connect(this, &QTabWidget::currentChanged, this, [this, tableView](int tabIndex) {
            if (widget(tabIndex) == tableView)
                emit selectionChanged(tableView->selectionModel()->selection());
        });

        addTab(tableView, str);
    }
}
//! [1]

//! [7]
bool AddressWidget::readFromFile()
{
    QFile file(fileName());

    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::information(this, tr("Unable to open file"),
                                 tr("Cannot open %1: %2").arg(QDir::toNativeSeparators(fileName()),
                                                              file.errorString()));
        return false;
    }

    QList<Contact> contacts;
    QDataStream in(&file);
    in >> contacts;

    if (contacts.isEmpty()) {
        QMessageBox::information(this, tr("No contacts in file"),
                                 tr("The file you are attempting to open contains no contacts."));
    }

    for (const auto &contact : std::as_const(contacts))
        addEntry(contact);

    return true;
}
//! [7]

//! [6]
bool AddressWidget::writeToFile()
{
    QFile file(fileName());

    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::information(this, tr("Unable to open file"),
                                 tr("Cannot write to %1: %2").arg(QDir::toNativeSeparators(fileName()),
                                                                  file.errorString()));
        return false;
    }

    auto sortedContacts = table->getContacts();
    std::sort(sortedContacts.begin(), sortedContacts.end());

    QDataStream out(&file);
    out << sortedContacts;
    return true;
}
//! [6]
