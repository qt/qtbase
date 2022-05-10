// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef CURRENCY_H
#define CURRENCY_H

#include <QWidget>
#include <QLocale>

class QLabel;
class QLineEdit;

class CurrencyWidget : public QWidget
{
    Q_OBJECT
public:
    CurrencyWidget();

private:
    QLabel *currencySymbolLabel;
    QLineEdit *currencySymbol;
    QLabel *currencyISOLabel;
    QLineEdit *currencyISO;
    QLabel *currencyNameLabel;
    QLineEdit *currencyName;
    QLabel *currencyFormattingLabel;
    QLineEdit *currencyFormattingValue;
    QLabel *currencyFormattingSymbolLabel;
    QLineEdit *currencyFormattingSymbol;
    QLineEdit *currencyFormatting;

private slots:
    void localeChanged(QLocale locale);
    void updateCurrencyFormatting();
};

#endif
