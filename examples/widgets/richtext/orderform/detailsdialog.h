// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef DETAILSDIALOG_H
#define DETAILSDIALOG_H

#include <QDialog>
#include <QList>
#include <QPair>

QT_BEGIN_NAMESPACE
class QCheckBox;
class QDialogButtonBox;
class QLabel;
class QLineEdit;
class QTableWidget;
class QTextEdit;
class QWidget;
QT_END_NAMESPACE

//! [0]
class DetailsDialog : public QDialog
{
    Q_OBJECT

public:
    DetailsDialog(const QString &title, QWidget *parent);

public slots:
    void verify();

public:
    QList<QPair<QString, int> > orderItems();
    QString senderName() const;
    QString senderAddress() const;
    bool sendOffers();

private:
    void setupItemsTable();

    QLabel *nameLabel;
    QLabel *addressLabel;
    QCheckBox *offersCheckBox;
    QLineEdit *nameEdit;
    QStringList items;
    QTableWidget *itemsTable;
    QTextEdit *addressEdit;
    QDialogButtonBox *buttonBox;
};
//! [0]

#endif // DETAILSDIALOG_H
