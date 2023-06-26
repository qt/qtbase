// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

#include "localeselector.h"

#include <QLocale>

LocaleSelector::LocaleSelector(QWidget *parent)
    : QComboBox(parent)
{
    int curIndex = -1;
    int index = 0;
    for (int _lang = QLocale::C; _lang <= QLocale::LastLanguage; ++_lang) {
        QLocale::Language lang = static_cast<QLocale::Language>(_lang);
        const QList<QLocale> locales =
                QLocale::matchingLocales(lang, QLocale::AnyScript, QLocale::AnyTerritory);
        for (const QLocale &l : locales) {
            QString label = QLocale::languageToString(l.language());
            label += QLatin1Char('/');
            label += QLocale::territoryToString(l.territory());
            // distinguish locales by script, if there are more than one script for a language/territory pair
            if (QLocale::matchingLocales(l.language(), QLocale::AnyScript, l.territory()).size() > 1)
                label += QLatin1String(" (") + QLocale::scriptToString(l.script()) + QLatin1Char(')');

            addItem(label, QVariant::fromValue(l));

            if (l.language() == locale().language() && l.territory() == locale().territory()
                && (locale().script() == QLocale::AnyScript || l.script() == locale().script())) {
                curIndex = index;
            }
            ++index;
        }
    }
    if (curIndex != -1)
        setCurrentIndex(curIndex);

    connect(this, QOverload<int>::of(&LocaleSelector::activated),
            this, &LocaleSelector::emitLocaleSelected);
}

void LocaleSelector::emitLocaleSelected(int index)
{
    QVariant v = itemData(index);
    if (!v.isValid())
        return;
    const QLocale l = qvariant_cast<QLocale>(v);
    emit localeSelected(l);
}
