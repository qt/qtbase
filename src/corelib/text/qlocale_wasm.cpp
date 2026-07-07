// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qlocale_p.h"

#include <emscripten.h>

QT_BEGIN_NAMESPACE

#ifndef QT_NO_SYSTEMLOCALE

namespace {

QStringList navigatorLanguages()
{
    // Read navigator.languages using EM_ASM_PTR instead of emscripten::val. The
    // latter does not support being used from static constructors (emscripten #23170)
    char *joined = reinterpret_cast<char *>(EM_ASM_PTR({
        return stringToNewUTF8(navigator.languages.join("\n"));
    }));
    const QStringList qtLanguages =
            QString::fromUtf8(joined).split(u'\n', Qt::SkipEmptyParts);
    free(joined);
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
