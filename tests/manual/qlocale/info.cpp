// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "info.h"

#include <QLineEdit>
#include <QScrollArea>
#include <QGridLayout>
#include <QLabel>
#include <QLocale>

InfoWidget::InfoWidget()
{
    scrollArea = new QScrollArea;
    scrollAreaWidget = new QWidget;
    scrollArea->setWidget(scrollAreaWidget);
    scrollArea->setWidgetResizable(true);
    layout = new QGridLayout();
    QVBoxLayout *v = new QVBoxLayout(scrollAreaWidget);
    v->addLayout(layout);
    v->addStretch();

    QVBoxLayout *l = new QVBoxLayout(this);
    l->addWidget(scrollArea);

    name = addItem("Name:");
    bcp47Name = addItem("Bcp47 name:");
    languageName = addItem("Language name:");
    nativeLanguageName = addItem("Native language name:");
    scriptName = addItem("Script name:");
    territoryName = addItem("Territory name:");
    nativeTerritoryName = addItem("Native territory name:");
}

void InfoWidget::localeChanged(QLocale locale)
{
    setLocale(locale);
    name->setText(locale.name());
    bcp47Name->setText(locale.bcp47Name());
    languageName->setText(QLocale::languageToString(locale.language()));
    nativeLanguageName->setText(locale.nativeLanguageName());
    scriptName->setText(QLocale::scriptToString(locale.script()));
    territoryName->setText(QLocale::territoryToString(locale.territory()));
    nativeTerritoryName->setText(locale.nativeTerritoryName());
}

void InfoWidget::addItem(const QString &label, QWidget *w)
{
    QLabel *lbl = new QLabel(label);
    int row = layout->rowCount();
    layout->addWidget(lbl, row, 0);
    layout->addWidget(w, row, 1, 1, 2);
}

QLineEdit *InfoWidget::addItem(const QString &label)
{
    QLineEdit *le = new QLineEdit;
    le->setReadOnly(true);
    addItem(label, le);
    return le;
}
