/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** You may use this file under the terms of the BSD license as follows:
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
**   * Neither the name of Digia Plc and its Subsidiary(-ies) nor the names
**     of its contributors may be used to endorse or promote products derived
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
