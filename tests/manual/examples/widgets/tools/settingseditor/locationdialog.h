// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef LOCATIONDIALOG_H
#define LOCATIONDIALOG_H

#include <QDialog>
#include <QSettings>

QT_BEGIN_NAMESPACE
class QComboBox;
class QDialogButtonBox;
class QGroupBox;
class QLabel;
class QTableWidget;
class QTableWidgetItem;
QT_END_NAMESPACE

class LocationDialog : public QDialog
{
    Q_OBJECT

public:
    LocationDialog(QWidget *parent = nullptr);

    QSettings::Format format() const;
    QSettings::Scope scope() const;
    QString organization() const;
    QString application() const;

private slots:
    void updateLocationsTable();
    void itemActivated(QTableWidgetItem *);

private:
    QLabel *formatLabel;
    QLabel *scopeLabel;
    QLabel *organizationLabel;
    QLabel *applicationLabel;
    QComboBox *formatComboBox;
    QComboBox *scopeComboBox;
    QComboBox *organizationComboBox;
    QComboBox *applicationComboBox;
    QGroupBox *locationsGroupBox;
    QTableWidget *locationsTable;
    QDialogButtonBox *buttonBox;
};

#endif
