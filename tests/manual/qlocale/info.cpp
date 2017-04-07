/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

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
    countryName = addItem("Country name:");
    nativeCountryName = addItem("Native country name:");
}

void InfoWidget::localeChanged(QLocale locale)
{
    setLocale(locale);
    name->setText(locale.name());
    bcp47Name->setText(locale.bcp47Name());
    languageName->setText(QLocale::languageToString(locale.language()));
    nativeLanguageName->setText(locale.nativeLanguageName());
    scriptName->setText(QLocale::scriptToString(locale.script()));
    countryName->setText(QLocale::countryToString(locale.country()));
    nativeCountryName->setText(locale.nativeCountryName());
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
