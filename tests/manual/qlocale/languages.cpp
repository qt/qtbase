// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#undef QT_NO_FOREACH // this file contains unported legacy Q_FOREACH uses

#include "languages.h"

#include <QLabel>
#include <QListWidget>
#include <QHBoxLayout>

LanguagesWidget::LanguagesWidget()
{
    QVBoxLayout *l = new QVBoxLayout(this);

    languagesLabel = new QLabel("Preferred languages:");
    languagesList = new QListWidget;

    l->addWidget(languagesLabel);
    l->addWidget(languagesList);

    localeChanged(QLocale());
}

void LanguagesWidget::localeChanged(QLocale locale)
{
    languagesList->clear();
    foreach (const QString &lang, locale.uiLanguages()) {
        QListWidgetItem *item = new QListWidgetItem(lang, languagesList);
        QLocale l(lang);
        if (l.language() != QLocale::C) {
            QString language = QLocale::languageToString(l.language());
            QString country = QLocale::territoryToString(l.territory());
            QString tooltip = QString(QLatin1String("%1: %2/%3")).arg(l.name(), language, country);
            item->setToolTip(tooltip);
        }
    }
}


