// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QWidget>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QCheckBox;
class QComboBox;
class QDateEdit;
class QGroupBox;
class QLabel;
class QLineEdit;
class QTreeView;
QT_END_NAMESPACE
class MySortFilterProxyModel;
class FilterWidget;
//! [0]
class Window : public QWidget
{
    Q_OBJECT

public:
    Window();

    void setSourceModel(QAbstractItemModel *model);

private slots:
    void textFilterChanged();
    void dateFilterChanged();

private:
    MySortFilterProxyModel *proxyModel;

    QGroupBox *sourceGroupBox;
    QGroupBox *proxyGroupBox;
    QTreeView *sourceView;
    QTreeView *proxyView;
    QLabel *filterPatternLabel;
    QLabel *fromLabel;
    QLabel *toLabel;
    FilterWidget *filterWidget;
    QDateEdit *fromDateEdit;
    QDateEdit *toDateEdit;
};
//! [0]

#endif // WINDOW_H
