// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "window.h"

#include <QtWidgets/QCheckBox>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QFormLayout>
#include <QtWidgets/QGroupBox>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QLabel>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QStyle>
#include <QtWidgets/QTreeView>

#include <QtGui/QColor>
#include <QtGui/QScreen>

#include <QtCore/QRegularExpression>
#include <QtCore/QSortFilterProxyModel>

using namespace Qt::StringLiterals;

static inline QColor textColor(const QPalette &palette)
{
    return palette.color(QPalette::Active, QPalette::Text);
}

static void setTextColor(QWidget *w, const QColor &c)
{
    auto palette = w->palette();
    if (textColor(palette) != c) {
        palette.setColor(QPalette::Active, QPalette::Text, c);
        w->setPalette(palette);
    }
}

Window::Window()
    : proxyModel(new QSortFilterProxyModel),
      sourceView(new QTreeView),
      proxyView(new QTreeView),
      sortCaseSensitivityCheckBox(new QCheckBox(tr("Case sensitive sorting"))),
      filterCaseSensitivityCheckBox(new QCheckBox(tr("Case sensitive filter"))),
      filterPatternLineEdit(new QLineEdit),
      filterSyntaxComboBox(new QComboBox),
      filterColumnComboBox(new QComboBox)
{
    sourceView->setRootIsDecorated(false);
    sourceView->setAlternatingRowColors(true);

    proxyView->setRootIsDecorated(false);
    proxyView->setAlternatingRowColors(true);
    proxyView->setModel(proxyModel);
    proxyView->setSortingEnabled(true);

    filterPatternLineEdit->setClearButtonEnabled(true);

    filterSyntaxComboBox->addItem(tr("Regular expression"), RegularExpression);
    filterSyntaxComboBox->addItem(tr("Wildcard"), Wildcard);
    filterSyntaxComboBox->addItem(tr("Fixed string"), FixedString);

    filterColumnComboBox->addItem(tr("Subject"));
    filterColumnComboBox->addItem(tr("Sender"));
    filterColumnComboBox->addItem(tr("Date"));

    connect(filterPatternLineEdit, &QLineEdit::textChanged,
            this, &Window::filterRegularExpressionChanged);
    connect(filterSyntaxComboBox, &QComboBox::currentIndexChanged,
            this, &Window::filterRegularExpressionChanged);
    connect(filterColumnComboBox, &QComboBox::currentIndexChanged,
            this, &Window::filterColumnChanged);
    connect(filterCaseSensitivityCheckBox, &QAbstractButton::toggled,
            this, &Window::filterRegularExpressionChanged);
    connect(sortCaseSensitivityCheckBox, &QAbstractButton::toggled,
            this, &Window::sortChanged);

    auto *sourceGroupBox = new QGroupBox(tr("Original Model"));
    auto *proxyGroupBox = new QGroupBox(tr("Sorted/Filtered Model"));

    auto *sourceLayout = new QHBoxLayout(sourceGroupBox);
    sourceLayout->addWidget(sourceView);

    auto *proxyLayout = new QVBoxLayout(proxyGroupBox);
    proxyLayout->addWidget(proxyView);

    auto *filterLayout = new QFormLayout;
    filterLayout->addRow(tr("&Filter pattern:"), filterPatternLineEdit);
    filterLayout->addRow(tr("Filter &syntax:"), filterSyntaxComboBox);
    filterLayout->addRow(tr("Filter &column:"), filterColumnComboBox);
    proxyLayout->addLayout(filterLayout);
    auto *checkBoxLayout = new QHBoxLayout();
    checkBoxLayout->addWidget(filterCaseSensitivityCheckBox);
    checkBoxLayout->addWidget(sortCaseSensitivityCheckBox);
    checkBoxLayout->addStretch();
    proxyLayout->addLayout(checkBoxLayout);

    auto *mainLayout = new QVBoxLayout(this);
    mainLayout->addWidget(sourceGroupBox);
    mainLayout->addWidget(proxyGroupBox);

    setWindowTitle(tr("Basic Sort/Filter Model"));
    auto screenGeometry = screen()->geometry();
    resize(screenGeometry.width() / 2, screenGeometry.height() * 2 / 3);

    proxyView->sortByColumn(1, Qt::AscendingOrder);
    filterColumnComboBox->setCurrentIndex(1);

    filterPatternLineEdit->setText(u"Andy|Grace"_s);
    filterCaseSensitivityCheckBox->setChecked(true);
    sortCaseSensitivityCheckBox->setChecked(true);
}

void Window::setSourceModel(QAbstractItemModel *model)
{
    proxyModel->setSourceModel(model);
    sourceView->setModel(model);

    for (int i = 0; i < proxyModel->columnCount(); ++i)
        proxyView->resizeColumnToContents(i);
    for (int i = 0; i < model->columnCount(); ++i)
        sourceView->resizeColumnToContents(i);
}

void Window::filterRegularExpressionChanged()
{
    Syntax s = Syntax(filterSyntaxComboBox->itemData(filterSyntaxComboBox->currentIndex()).toInt());
    QString pattern = filterPatternLineEdit->text();
    switch (s) {
    case Wildcard:
        pattern = QRegularExpression::wildcardToRegularExpression(pattern);
        break;
    case FixedString:
        pattern = QRegularExpression::escape(pattern);
        break;
    default:
        break;
    }

    QRegularExpression::PatternOptions options = QRegularExpression::NoPatternOption;
    if (!filterCaseSensitivityCheckBox->isChecked())
        options |= QRegularExpression::CaseInsensitiveOption;
    QRegularExpression regularExpression(pattern, options);

    if (regularExpression.isValid()) {
        filterPatternLineEdit->setToolTip(QString());
        proxyModel->setFilterRegularExpression(regularExpression);
        setTextColor(filterPatternLineEdit, textColor(style()->standardPalette()));
    } else {
        filterPatternLineEdit->setToolTip(regularExpression.errorString());
        proxyModel->setFilterRegularExpression(QRegularExpression());
        setTextColor(filterPatternLineEdit, Qt::red);
    }
}

void Window::filterColumnChanged()
{
    proxyModel->setFilterKeyColumn(filterColumnComboBox->currentIndex());
}

void Window::sortChanged()
{
    proxyModel->setSortCaseSensitivity(
            sortCaseSensitivityCheckBox->isChecked() ? Qt::CaseSensitive
                                                     : Qt::CaseInsensitive);
}
