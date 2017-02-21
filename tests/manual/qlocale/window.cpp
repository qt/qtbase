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

#include "window.h"

#include <QComboBox>
#include <QLocale>
#include <QLabel>
#include <QTabWidget>
#include <QHBoxLayout>
#include <QEvent>

Window::Window()
{
    localeCombo = new QComboBox;

    localeCombo->addItem("System", QLocale::system());

    QList<QLocale> locales = QLocale::matchingLocales(QLocale::AnyLanguage, QLocale::AnyScript, QLocale::AnyCountry);
    foreach (const QLocale &locale, locales) {
        QString label = QLocale::languageToString(locale.language());
        label += QLatin1Char('/');
        if (locale.script() != QLocale::AnyScript) {
            label += QLocale::scriptToString(locale.script());
            label += QLatin1Char('/');
        }
        label += QLocale::countryToString(locale.country());
        localeCombo->addItem(label, locale);
    }

    connect(localeCombo, SIGNAL(currentIndexChanged(int)),
            this, SLOT(localeChanged(int)));

    tabWidget = new QTabWidget;
    info = new InfoWidget;
    connect(this, SIGNAL(localeChanged(QLocale)), info, SLOT(localeChanged(QLocale)));
    calendar = new CalendarWidget;
    connect(this, SIGNAL(localeChanged(QLocale)), calendar, SLOT(localeChanged(QLocale)));
    currency = new CurrencyWidget;
    connect(this, SIGNAL(localeChanged(QLocale)), currency, SLOT(localeChanged(QLocale)));
    languages = new LanguagesWidget;
    connect(this, SIGNAL(localeChanged(QLocale)), languages, SLOT(localeChanged(QLocale)));
    dateFormats = new DateFormatsWidget;
    connect(this, SIGNAL(localeChanged(QLocale)), dateFormats, SLOT(localeChanged(QLocale)));
    numberFormats = new NumberFormatsWidget;
    connect(this, SIGNAL(localeChanged(QLocale)), numberFormats, SLOT(localeChanged(QLocale)));
    miscellaneous = new MiscWidget;
    connect(this, SIGNAL(localeChanged(QLocale)), miscellaneous, SLOT(localeChanged(QLocale)));

    localeName = new QLabel("Locale: foo_BAR");

    QWidget *w = new QWidget;
    QHBoxLayout *headerLayout = new QHBoxLayout(w);
    headerLayout->addWidget(localeCombo);
    headerLayout->addWidget(localeName);

    QWidget *central = new QWidget;
    QVBoxLayout *l = new QVBoxLayout(central);
    l->addWidget(w);
    l->addWidget(tabWidget);

    tabWidget->addTab(info, "Info");
    tabWidget->addTab(calendar, "Calendar");
    tabWidget->addTab(currency, "Currency");
    tabWidget->addTab(languages, "Languages");
    tabWidget->addTab(dateFormats, "Date Formats");
    tabWidget->addTab(numberFormats, "Number Formats");
    tabWidget->addTab(miscellaneous, "Text");

    localeCombo->setCurrentIndex(0);
    systemLocaleChanged();

    setCentralWidget(central);
}

void Window::systemLocaleChanged()
{
    QLocale l = QLocale::system();
    QString lang = QLocale::languageToString(l.language());
    QString script = QLocale::scriptToString(l.script());
    QString country = QLocale::countryToString(l.country());
    if (l.script() != QLocale::AnyScript)
        localeCombo->setItemText(0, QString("System: %1-%2-%3").arg(lang, script, country));
    else
        localeCombo->setItemText(0, QString("System: %1-%2").arg(lang, country));
    emit localeChanged(0);
}

void Window::localeChanged(int idx)
{
    QLocale locale = localeCombo->itemData(idx).toLocale();
    localeName->setText(QString("Locale: %1 (%2)").arg(locale.bcp47Name(), locale.name()));
    emit localeChanged(locale);
}

bool Window::event(QEvent *event)
{
    switch (event->type()) {
    case QEvent::LocaleChange: {
        if (localeCombo->currentIndex() == 0)
            systemLocaleChanged();
        return true;
    }
    default:
        break;
    }
    return QMainWindow::event(event);
}
