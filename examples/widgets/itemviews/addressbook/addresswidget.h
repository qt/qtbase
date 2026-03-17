// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ADDRESSWIDGET_H
#define ADDRESSWIDGET_H

#include "contact.h"

#include <QTabWidget>

#include <QList>
#include <QRangeModelAdapter>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
class QItemSelection;
QT_END_NAMESPACE

struct Contact;
class NewAddressTab;
class TableModel;

//! [RowOptions declaration]
template <>
struct QRangeModel::RowOptions<Contact>
{
    inline static QVariant headerData(int section, int role);
};
//! [RowOptions declaration]

//! [0]
class AddressWidget : public QTabWidget
{
    Q_OBJECT

public:
    explicit AddressWidget(QWidget *parent = nullptr);

    bool readFromFile();
    bool writeToFile();

    static QString fileName();

public slots:
    void showAddEntryDialog();
    void addEntry(const Contact &contact);
    void editEntry();
    void removeEntry();

signals:
    void selectionChanged (const QItemSelection &selected);

private:
    void setupTabs();

    NewAddressTab *newAddressTab;

    QList<Contact> contacts;
    using Adapter = decltype(QRangeModelAdapter(std::ref(contacts)));
    Adapter adapter;
};
//! [0]

//! [RowOptions headerData]
QVariant QRangeModel::RowOptions<Contact>::headerData(int section, int role)
{
    if (role == Qt::DisplayRole) {
        switch (section) {
        case 0:
            return AddressWidget::tr("Name");
        case 1:
            return AddressWidget::tr("Address");
        default:
            break;
        }
    }
    return {};
}
//! [RowOptions headerData]

#endif // ADDRESSWIDGET_H
