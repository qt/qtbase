/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "currency.h"
#include <QLineEdit>
#include <QLabel>
#include <QHBoxLayout>
#include <QLocale>

CurrencyWidget::CurrencyWidget()
{
    QGridLayout *l = new QGridLayout;

    currencySymbolLabel = new QLabel("Symbol:");
    currencySymbol = new QLineEdit;
    currencyISOLabel = new QLabel("ISO Code:");
    currencyISO = new QLineEdit;
    currencyNameLabel = new QLabel("Display name:");
    currencyName = new QLineEdit;
    currencyFormattingLabel = new QLabel("Currency formatting:");
    currencyFormattingValue = new QLineEdit(QString::number(1234.56, 'f', 2));
    currencyFormattingSymbolLabel = new QLabel("currency:");
    currencyFormattingSymbol = new QLineEdit;
    currencyFormatting = new QLineEdit;

    currencyFormattingValue->setFixedWidth(150);
    currencyFormattingSymbol->setFixedWidth(50);

    l->addWidget(currencySymbolLabel, 0, 0);
    l->addWidget(currencySymbol, 0, 1, 1, 4);
    l->addWidget(currencyISOLabel, 1, 0);
    l->addWidget(currencyISO, 1, 1, 1, 4);
    l->addWidget(currencyNameLabel, 2, 0);
    l->addWidget(currencyName, 2, 1, 1, 4);
    l->addWidget(currencyFormattingLabel, 3, 0);
    l->addWidget(currencyFormattingValue, 3, 1);
    l->addWidget(currencyFormattingSymbolLabel, 3, 2);
    l->addWidget(currencyFormattingSymbol, 3, 3);
    l->addWidget(currencyFormatting, 3, 4);

    QVBoxLayout *v = new QVBoxLayout(this);
    v->addLayout(l);
    v->addStretch();

    connect(currencyFormattingSymbol, SIGNAL(textChanged(QString)),
            this, SLOT(updateCurrencyFormatting()));
    connect(currencyFormattingValue, SIGNAL(textChanged(QString)),
            this, SLOT(updateCurrencyFormatting()));
}

void CurrencyWidget::updateCurrencyFormatting()
{
    QString result;
    bool ok;
    QString symbol = currencyFormattingSymbol->text();
    QString value = currencyFormattingValue->text();
    int i = value.toInt(&ok);
    if (ok) {
        result = locale().toCurrencyString(i, symbol);
    } else {
        double d = value.toDouble(&ok);
        if (ok)
            result = locale().toCurrencyString(d, symbol);
    }
    currencyFormatting->setText(result);
}

void CurrencyWidget::localeChanged(QLocale locale)
{
    setLocale(locale);
    currencySymbol->setText(locale.currencySymbol());
    currencyISO->setText(locale.currencySymbol(QLocale::CurrencyIsoCode));
    currencyName->setText(locale.currencySymbol(QLocale::CurrencyDisplayName));
    updateCurrencyFormatting();
}

