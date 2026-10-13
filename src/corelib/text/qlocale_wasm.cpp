// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qlocale_p.h"

#include <emscripten/val.h>

#include <string>
#include <vector>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SYSTEMLOCALE

namespace {

QStringList navigatorLanguages()
{
    using emscripten::val;
    const val navigator = val::global("navigator");
    const auto languages = emscripten::vecFromJSArray<std::string>(navigator["languages"]);
    QStringList qtLanguages;
    for (const std::string& language : languages)
        qtLanguages.append(QString::fromStdString(language));
    return qtLanguages;
}

}

QVariant QSystemLocale::query(QueryType query, QVariant &&in) const
{
    Q_UNUSED(in);

    switch (query) {
    case QSystemLocale::UILanguages:
        return QVariant(navigatorLanguages());
    default:
    break;
    }

    return QVariant();
}

QLocale QSystemLocale::fallbackLocale() const
{
    const QStringList languages = navigatorLanguages();
    if (languages.isEmpty())
        return QLocale(u"en-US");
    return QLocale(languages[0]);
}

#endif // QT_NO_SYSTEMLOCALE

QT_END_NAMESPACE
