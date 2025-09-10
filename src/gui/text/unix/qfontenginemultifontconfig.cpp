// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qfontenginemultifontconfig_p.h"

#include <QtGui/private/qfontengine_ft_p.h>

QT_BEGIN_NAMESPACE

QFontEngineMultiFontConfig::QFontEngineMultiFontConfig(QFontEngine *fe, int script)
    : QFontEngineMulti(fe, script)
{
}

QFontEngineMultiFontConfig::~QFontEngineMultiFontConfig()
{
    for (FcPattern *pattern : std::as_const(cachedMatchPatterns)) {
        if (pattern)
            FcPatternDestroy(pattern);
    }
}

bool QFontEngineMultiFontConfig::shouldLoadFontEngineForCharacter(int at, uint ucs4) const
{
    bool charSetHasChar = true;
    FcPattern *matchPattern = getMatchPatternForFallback(at - 1);
    if (matchPattern != nullptr) {
        FcCharSet *charSet;
        FcPatternGetCharSet(matchPattern, FC_CHARSET, 0, &charSet);
        charSetHasChar = FcCharSetHasChar(charSet, ucs4);
    }

    return charSetHasChar;
}


FcPattern * QFontEngineMultiFontConfig::getMatchPatternForFallback(int fallBackIndex) const
{
    Q_ASSERT(fallBackIndex < fallbackFamilyCount());
    if (fallbackFamilyCount() > cachedMatchPatterns.size())
        cachedMatchPatterns.resize(fallbackFamilyCount());
    FcPattern *ret = cachedMatchPatterns.at(fallBackIndex);
    if (ret)
        return ret;
    FcPattern *requestPattern = FcPatternCreate();
    FcValue value;
    value.type = FcTypeString;
    QByteArray cs = fallbackFamilyAt(fallBackIndex).toUtf8();
    value.u.s = reinterpret_cast<const FcChar8 *>(cs.data());
    FcPatternAdd(requestPattern, FC_FAMILY, value, true);
    FcResult result;
    ret = FcFontMatch(nullptr, requestPattern, &result);
    cachedMatchPatterns.insert(fallBackIndex, ret);
    FcPatternDestroy(requestPattern);
    return ret;
}

QT_END_NAMESPACE
