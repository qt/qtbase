// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef ADDRESSWIDGET_H
#define ADDRESSWIDGET_H

#include <QTabWidget>

QT_BEGIN_NAMESPACE
class QSortFilterProxyModel;
class QItemSelection;
QT_END_NAMESPACE

struct Contact;
class NewAddressTab;
class TableModel;

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

    TableModel *table;
    NewAddressTab *newAddressTab;
};
//! [0]

#endif // ADDRESSWIDGET_H
