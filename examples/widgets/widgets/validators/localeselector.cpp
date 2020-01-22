/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the examples of the Qt Toolkit.
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
                QLocale::matchingLocales(lang, QLocale::AnyScript, QLocale::AnyCountry);
        for (const QLocale &l : locales) {
            QString label = QLocale::languageToString(l.language());
            label += QLatin1Char('/');
            label += QLocale::countryToString(l.country());
            // distinguish locales by script, if there are more than one script for a language/country pair
            if (QLocale::matchingLocales(l.language(), QLocale::AnyScript, l.country()).size() > 1)
                label += QLatin1String(" (") + QLocale::scriptToString(l.script()) + QLatin1Char(')');

            addItem(label, QVariant::fromValue(l));

            if (l.language() == locale().language() && l.country() == locale().country()
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
