// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef WINDOW_H
#define WINDOW_H

#include <QMainWindow>

#include "calendar.h"
#include "currency.h"
#include "languages.h"
#include "dateformats.h"
#include "numberformats.h"
#include "miscellaneous.h"
#include "info.h"

class QLabel;
class QComboBox;

class Window : public QMainWindow
{
    Q_OBJECT
public:
    Window();

    QLabel *localeName;
    QComboBox *localeCombo;
    QTabWidget *tabWidget;
    CalendarWidget *calendar;
    CurrencyWidget *currency;
    LanguagesWidget *languages;
    DateFormatsWidget *dateFormats;
    NumberFormatsWidget *numberFormats;
    MiscWidget *miscellaneous;
    InfoWidget *info;

private:
    bool event(QEvent *);
    void systemLocaleChanged();

signals:
    void localeChanged(QLocale);

private slots:
    void localeChanged(int);
};

#endif
