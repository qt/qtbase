// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#ifndef WINDOW_H
#define WINDOW_H

#include <QtWidgets/QWidget>

QT_BEGIN_NAMESPACE
class QAbstractItemModel;
class QCheckBox;
class QComboBox;
class QLineEdit;
class QSortFilterProxyModel;
class QTreeView;
QT_END_NAMESPACE

class Window : public QWidget
{
    Q_OBJECT

public:
    Window();

    void setSourceModel(QAbstractItemModel *model);

private slots:
    void filterRegularExpressionChanged();
    void filterColumnChanged();
    void sortChanged();

private:
    QSortFilterProxyModel *proxyModel;

    QTreeView *sourceView;
    QTreeView *proxyView;
    QCheckBox *sortCaseSensitivityCheckBox;
    QCheckBox *filterCaseSensitivityCheckBox;
    QLineEdit *filterPatternLineEdit;
    enum Syntax {
        RegularExpression,
        Wildcard,
        FixedString
    };

    QComboBox *filterSyntaxComboBox;
    QComboBox *filterColumnComboBox;
};

#endif // WINDOW_H
