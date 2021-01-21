/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

#include "qfontenginemultifontconfig_p.h"

#include <QtFontDatabaseSupport/private/qfontengine_ft_p.h>

QT_BEGIN_NAMESPACE

QFontEngineMultiFontConfig::QFontEngineMultiFontConfig(QFontEngine *fe, int script)
    : QFontEngineMulti(fe, script)
{
}

QFontEngineMultiFontConfig::~QFontEngineMultiFontConfig()
{
    for (FcPattern *pattern : qAsConst(cachedMatchPatterns)) {
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
