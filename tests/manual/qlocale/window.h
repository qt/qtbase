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
