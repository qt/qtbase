// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#define QFONTENGINE_COMMON
#include "../../../../auto/gui/text/qfontengine/tst_qfontengine.cpp"

using namespace Qt::StringLiterals;

void tst_QFontEngine::glyphCount()
{
    QFETCH(const Candidate, candidate);
    if (!candidate.isFontAvailable())
        QSKIP("Font is not available");

    const auto fontEngine = candidate.fontEngine();

    QBENCHMARK {
        fontEngine->glyphCount();
    }
}

void tst_QFontEngine::glyphIndex()
{
    QFETCH(const Candidate, candidate);
    if (!candidate.isFontAvailable())
        QSKIP("Font is not available");

    const auto fontEngine = candidate.fontEngine();

    QBENCHMARK {
        fontEngine->glyphIndex(candidate.ucs4);
    }
}

void tst_QFontEngine::glyphName()
{
    QFETCH(const Candidate, candidate);
    if (!candidate.isFontAvailable())
        QSKIP("Font is not available");

    const auto fontEngine = candidate.fontEngine();

    QBENCHMARK {
        fontEngine->glyphName(candidate.expectedGlyphIndex);
    }
}

void tst_QFontEngine::findGlyph()
{
    QFETCH(const Candidate, candidate);
    if (!candidate.isFontAvailable())
        QSKIP("Font is not available");

    const auto fontEngine = candidate.fontEngine();
    const auto glyphName = candidate.glyphName();

    QBENCHMARK {
        fontEngine->findGlyph(glyphName);
    }
}
