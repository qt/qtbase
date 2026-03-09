// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "window.h"
#include "mysortfilterproxymodel.h"
#include "filterwidget.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDateEdit>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QStyle>
#include <QtWidgets/QTreeView>

#include <QtGui/QScreen>

#include <QtCore/QDate>

//! [0]
Window::Window()
    : proxyModel(new MySortFilterProxyModel(this)),
//! [0]
      sourceView(new QTreeView),
      proxyView(new QTreeView),
      filterWidget(new FilterWidget),
      fromDateEdit(new QDateEdit),
      toDateEdit(new QDateEdit)
{
    //! [1]
    sourceView->setRootIsDecorated(false);
    sourceView->setAlternatingRowColors(true);
    //! [1]

    //! [2]
    auto *sourceGroupBox = new QGroupBox(tr("Original Model"));
    auto *sourceLayout = new QHBoxLayout(sourceGroupBox);
    sourceLayout->addWidget(sourceView);
    //! [2]

    //! [3]
    filterWidget->setText(tr("Grace|Sports"));
    fromDateEdit->setDate(QDate(1970, 01, 01));
    toDateEdit->setDate(QDate(2099, 12, 31));
    //! [3]
    //! [4]
    connect(filterWidget, &FilterWidget::filterChanged,
            this, &Window::textFilterChanged);
    connect(filterWidget, &QLineEdit::textChanged,
            this, &Window::textFilterChanged);
    connect(fromDateEdit, &QDateTimeEdit::dateChanged,
            this, &Window::dateFilterChanged);
    connect(toDateEdit, &QDateTimeEdit::dateChanged,
            this, &Window::dateFilterChanged);
    //! [4]

    //! [5]
    proxyView->setRootIsDecorated(false);
    proxyView->setAlternatingRowColors(true);
    proxyView->setModel(proxyModel);
    proxyView->setSortingEnabled(true);
    proxyView->sortByColumn(1, Qt::AscendingOrder);

    auto *proxyGroupBox = new QGroupBox(tr("Sorted/Filtered Model"));
    auto *proxyLayout = new QVBoxLayout(proxyGroupBox);
    proxyLayout->addWidget(proxyView);
    auto *formLayout = new QFormLayout;
    proxyLayout->addLayout(formLayout);
    formLayout->addRow(tr("&Filter pattern:"), filterWidget);
    formLayout->addRow(tr("F&rom:"), fromDateEdit);
    formLayout->addRow(tr("&To:"), toDateEdit);

    //! [5]

    //! [6]
    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(sourceGroupBox);
    mainLayout->addWidget(proxyGroupBox);

    setWindowTitle(tr("Custom Sort/Filter Model"));
    auto screenGeometry = screen()->geometry();
    resize(screenGeometry.width() / 2, screenGeometry.height() * 2 / 3);
}
//! [6]

//! [7]
void Window::setSourceModel(QAbstractItemModel *model)
{
    proxyModel->setSourceModel(model);
    sourceView->setModel(model);

    for (int i = 0; i < proxyModel->columnCount(); ++i)
        proxyView->resizeColumnToContents(i);
    for (int i = 0; i < model->columnCount(); ++i)
        sourceView->resizeColumnToContents(i);
}
//! [7]

//! [8]
void Window::textFilterChanged()
{
    FilterWidget::PatternSyntax s = filterWidget->patternSyntax();
    QString pattern = filterWidget->text();
    switch (s) {
    case FilterWidget::Wildcard:
        pattern = QRegularExpression::wildcardToRegularExpression(pattern);
        break;
    case FilterWidget::FixedString:
        pattern = QRegularExpression::escape(pattern);
        break;
    default:
        break;
    }

    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (filterWidget->caseSensitivity() == Qt::CaseInsensitive)
        options |= QRegularExpression::CaseInsensitiveOption;
    QRegularExpression regularExpression(pattern, options);
    proxyModel->setFilterRegularExpression(regularExpression);
}
//! [8]

//! [9]
void Window::dateFilterChanged()
{
    proxyModel->setFilterMinimumDate(fromDateEdit->date());
    proxyModel->setFilterMaximumDate(toDateEdit->date());
}
//! [9]
