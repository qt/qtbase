// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qunicodetools_p.h"

#include "qunicodetables_p.h"
#include "qvarlengtharray.h"
#if QT_CONFIG(library)
#include "qlibrary.h"
#endif

#include <limits.h>

#define FLAG(x) (1 << (x))

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#ifdef QT_BUILD_INTERNAL
Q_CONSTINIT Q_AUTOTEST_EXPORT
#else
constexpr
#endif
int qt_initcharattributes_default_algorithm_only = 0;

namespace QUnicodeTools {

// -----------------------------------------------------------------------------------------------------
//
// The text boundaries determination algorithm.
// See https://www.unicode.org/reports/tr29/tr29-37.html
//
// -----------------------------------------------------------------------------------------------------

namespace GB {

// This table is indexed by the grapheme break classes of two
// (adjacent) code points.
// The class of the first code point selects an entry.
// If the entry's bit at position second_cp_class is set
// (in other words: if entry & (1u << second_cp_class) is non-zero)
// then there is NO grapheme break between the two code points.

using GBTableEntryType = quint16;

// Check that we have enough bits in the table (in case
// NumGraphemeBreakClasses grows too much).
static_assert(sizeof(GBTableEntryType) * CHAR_BIT >= QUnicodeTables::NumGraphemeBreakClasses,
              "Internal error: increase the size in bits of GBTableEntryType");

// GB9, GB9a
static const GBTableEntryType Extend_SpacingMark_ZWJ =
        FLAG(QUnicodeTables::GraphemeBreak_Extend)
        | FLAG(QUnicodeTables::GraphemeBreak_SpacingMark)
        | FLAG(QUnicodeTables::GraphemeBreak_ZWJ);

static const GBTableEntryType HardBreak = 0u;

static const GBTableEntryType breakTable[QUnicodeTables::NumGraphemeBreakClasses] = {
    Extend_SpacingMark_ZWJ, // Any
    FLAG(QUnicodeTables::GraphemeBreak_LF), // CR
    HardBreak, // LF
    HardBreak, // Control
    Extend_SpacingMark_ZWJ, // Extend
    Extend_SpacingMark_ZWJ, // ZWJ
    Extend_SpacingMark_ZWJ, // RegionalIndicator
    (Extend_SpacingMark_ZWJ
        | FLAG(QUnicodeTables::GraphemeBreak_Any)
        | FLAG(QUnicodeTables::GraphemeBreak_Prepend)
        | FLAG(QUnicodeTables::GraphemeBreak_L)
        | FLAG(QUnicodeTables::GraphemeBreak_V)
        | FLAG(QUnicodeTables::GraphemeBreak_T)
        | FLAG(QUnicodeTables::GraphemeBreak_LV)
        | FLAG(QUnicodeTables::GraphemeBreak_LVT)
        | FLAG(QUnicodeTables::GraphemeBreak_RegionalIndicator)
        | FLAG(QUnicodeTables::GraphemeBreak_Extended_Pictographic)
    ), // Prepend
    Extend_SpacingMark_ZWJ, // SpacingMark
    (Extend_SpacingMark_ZWJ
        | FLAG(QUnicodeTables::GraphemeBreak_L)
        | FLAG(QUnicodeTables::GraphemeBreak_V)
        | FLAG(QUnicodeTables::GraphemeBreak_LV)
        | FLAG(QUnicodeTables::GraphemeBreak_LVT)
    ), // L
    (Extend_SpacingMark_ZWJ
        | FLAG(QUnicodeTables::GraphemeBreak_V)
        | FLAG(QUnicodeTables::GraphemeBreak_T)
    ), // V
    (Extend_SpacingMark_ZWJ
        | FLAG(QUnicodeTables::GraphemeBreak_T)
    ), // T
    (Extend_SpacingMark_ZWJ
        | FLAG(QUnicodeTables::GraphemeBreak_V)
        | FLAG(QUnicodeTables::GraphemeBreak_T)
    ), // LV
    (Extend_SpacingMark_ZWJ
        | FLAG(QUnicodeTables::GraphemeBreak_T)
    ), // LVT
    Extend_SpacingMark_ZWJ // Extended_Pictographic
};

static bool shouldBreakBetweenClasses(QUnicodeTables::GraphemeBreakClass first,
                                      QUnicodeTables::GraphemeBreakClass second)
{
    return (breakTable[first] & FLAG(second)) == 0;
}

// Some rules (GB11, GB12, GB13) cannot be represented by the table alone,
// so we need to store some local state.
enum class State : uchar {
    Normal,
    GB11_ExtPicExt,    // saw a Extend after a Extended_Pictographic
    GB11_ExtPicExtZWJ, // saw a ZWG after a Extended_Pictographic and zero or more Extend
    GB12_13_RI,        // saw a RegionalIndicator following a non-RegionalIndicator
};

} // namespace GB

static void getGraphemeBreaks(const char16_t *string, qsizetype len, QCharAttributes *attributes)
{
    QUnicodeTables::GraphemeBreakClass lcls = QUnicodeTables::GraphemeBreak_LF; // to meet GB1
    GB::State state = GB::State::Normal;
    for (qsizetype i = 0; i != len; ++i) {
        qsizetype pos = i;
        char32_t ucs4 = string[i];
        if (QChar::isHighSurrogate(ucs4) && i + 1 != len) {
            ushort low = string[i + 1];
            if (QChar::isLowSurrogate(low)) {
                ucs4 = QChar::surrogateToUcs4(ucs4, low);
                ++i;
            }
        }

        const QUnicodeTables::Properties *prop = QUnicodeTables::properties(ucs4);
        QUnicodeTables::GraphemeBreakClass cls = (QUnicodeTables::GraphemeBreakClass) prop->graphemeBreakClass;

        bool shouldBreak = GB::shouldBreakBetweenClasses(lcls, cls);
        bool handled = false;

        switch (state) {
        case GB::State::Normal:
            break; // will deal with it below

        case GB::State::GB11_ExtPicExt:
            Q_ASSERT(lcls == QUnicodeTables::GraphemeBreak_Extend);
            if (cls == QUnicodeTables::GraphemeBreak_Extend) {
                // keep going in the current state
                Q_ASSERT(!shouldBreak); // GB9, do not break before Extend
                handled = true;
            } else if (cls == QUnicodeTables::GraphemeBreak_ZWJ) {
                state = GB::State::GB11_ExtPicExtZWJ;
                Q_ASSERT(!shouldBreak); // GB9, do not break before ZWJ
                handled = true;
            } else {
                state = GB::State::Normal;
            }
            break;

        case GB::State::GB11_ExtPicExtZWJ:
            Q_ASSERT(lcls == QUnicodeTables::GraphemeBreak_ZWJ);
            if (cls == QUnicodeTables::GraphemeBreak_Extended_Pictographic) {
                shouldBreak = false;
                handled = true;
            }

            state = GB::State::Normal;
            break;

        case GB::State::GB12_13_RI:
            Q_ASSERT(lcls == QUnicodeTables::GraphemeBreak_RegionalIndicator);
            if (cls == QUnicodeTables::GraphemeBreak_RegionalIndicator) {
                shouldBreak = false;
                handled = true;
            }

            state = GB::State::Normal;
            break;
        }

        if (!handled) {
            Q_ASSERT(state == GB::State::Normal);
            if (lcls == QUnicodeTables::GraphemeBreak_Extended_Pictographic) { // GB11
                if (cls == QUnicodeTables::GraphemeBreak_Extend) {
                    state = GB::State::GB11_ExtPicExt;
                    Q_ASSERT(!shouldBreak); // GB9, do not break before Extend
                } else if (cls == QUnicodeTables::GraphemeBreak_ZWJ) {
                    state = GB::State::GB11_ExtPicExtZWJ;
                    Q_ASSERT(!shouldBreak); // GB9, do not break before ZWJ
                }
            } else if (cls == QUnicodeTables::GraphemeBreak_RegionalIndicator) { // GB12, GB13
                state = GB::State::GB12_13_RI;
            }
        }

        if (shouldBreak)
            attributes[pos].graphemeBoundary = true;

        lcls = cls;
    }

    attributes[len].graphemeBoundary = true; // GB2
}


namespace WB {

enum Action {
    NoBreak,
    Break,
    Lookup,
    LookupW
};

static const uchar breakTable[QUnicodeTables::NumWordBreakClasses][QUnicodeTables::NumWordBreakClasses] = {
//    Any      CR       LF       Newline  Extend   ZWJ      Format    RI       Katakana HLetter  ALetter  SQuote   DQuote  MidNumLet MidLetter MidNum  Numeric ExtNumLet WSeg
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // Any
    { Break  , Break  , NoBreak, Break  , Break  , Break  , Break  ,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // CR
    { Break  , Break  , Break  , Break  , Break  , Break  , Break  ,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // LF
    { Break  , Break  , Break  , Break  , Break  , Break  , Break  ,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // Newline
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // Extend
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // ZWJ
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // Format
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  NoBreak, Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // RegionalIndicator
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , NoBreak, Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , NoBreak, Break   }, // Katakana
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , NoBreak, NoBreak, LookupW, Lookup , LookupW, LookupW, Break  , NoBreak, NoBreak, Break   }, // HebrewLetter
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , NoBreak, NoBreak, LookupW, Break  , LookupW, LookupW, Break  , NoBreak, NoBreak, Break   }, // ALetter
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // SingleQuote
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // DoubleQuote
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // MidNumLet
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // MidLetter
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // MidNum
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , NoBreak, NoBreak, Lookup , Break  , Lookup , Break  , Lookup , NoBreak, NoBreak, Break   }, // Numeric
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , NoBreak, NoBreak, NoBreak, Break  , Break  , Break  , Break  , Break  , NoBreak, NoBreak, Break   }, // ExtendNumLet
    { Break  , Break  , Break  , Break  , NoBreak, NoBreak, NoBreak,  Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , NoBreak }, // WSegSpace
};

} // namespace WB

static void getWordBreaks(const char16_t *string, qsizetype len, QCharAttributes *attributes)
{
    enum WordType {
        WordTypeNone, WordTypeAlphaNumeric, WordTypeHiraganaKatakana
    } currentWordType = WordTypeNone;

    QUnicodeTables::WordBreakClass cls = QUnicodeTables::WordBreak_LF; // to meet WB1
    auto real_cls = cls; // Unaffected by WB4

    for (qsizetype i = 0; i != len; ++i) {
        qsizetype pos = i;
        char32_t ucs4 = string[i];
        if (QChar::isHighSurrogate(ucs4) && i + 1 != len) {
            ushort low = string[i + 1];
            if (QChar::isLowSurrogate(low)) {
                ucs4 = QChar::surrogateToUcs4(ucs4, low);
                ++i;
            }
        }

        const QUnicodeTables::Properties *prop = QUnicodeTables::properties(ucs4);
        QUnicodeTables::WordBreakClass ncls = (QUnicodeTables::WordBreakClass) prop->wordBreakClass;
        if (qt_initcharattributes_default_algorithm_only) {
            // as of Unicode 5.1, some punctuation marks were mapped to MidLetter and MidNumLet
            // which caused "hi.there" to be treated like if it were just a single word;
            // we keep the pre-5.1 behavior by remapping these characters in the Unicode tables generator
            // and this code is needed to pass the coverage tests; remove once the issue is fixed.
            if (ucs4 == 0x002E) // FULL STOP
                ncls = QUnicodeTables::WordBreak_MidNumLet;
            else if (ucs4 == 0x003A) // COLON
                ncls = QUnicodeTables::WordBreak_MidLetter;
        }

        uchar action = WB::breakTable[cls][ncls];
        switch (action) {
        case WB::Break:
            if (Q_UNLIKELY(real_cls == QUnicodeTables::WordBreak_ZWJ
                           && prop->graphemeBreakClass
                                   == QUnicodeTables::GraphemeBreak_Extended_Pictographic)) {
                // WB3c: ZWJ × \p{Extended_Pictographic}
                action = WB::NoBreak;
            }
            break;
        case WB::NoBreak:
            if (Q_UNLIKELY(ncls == QUnicodeTables::WordBreak_Extend || ncls == QUnicodeTables::WordBreak_ZWJ || ncls == QUnicodeTables::WordBreak_Format)) {
                // WB4: X(Extend|Format)* -> X
                real_cls = ncls;
                continue;
            }
            if (Q_UNLIKELY(cls == QUnicodeTables::WordBreak_RegionalIndicator)) {
                // WB15/WB16: break between pairs of Regional indicator
                ncls = QUnicodeTables::WordBreak_Any;
            }
            if (Q_UNLIKELY(ncls == QUnicodeTables::WordBreak_WSegSpace
                           && real_cls != QUnicodeTables::WordBreak_WSegSpace)) {
                // WB3d should not be affected by WB4
                action = WB::Break;
            }
            break;
        case WB::Lookup:
        case WB::LookupW:
            for (qsizetype lookahead = i + 1; lookahead < len; ++lookahead) {
                ucs4 = string[lookahead];
                if (QChar::isHighSurrogate(ucs4) && lookahead + 1 != len) {
                    ushort low = string[lookahead + 1];
                    if (QChar::isLowSurrogate(low)) {
                        ucs4 = QChar::surrogateToUcs4(ucs4, low);
                        ++lookahead;
                    }
                }

                prop = QUnicodeTables::properties(ucs4);
                QUnicodeTables::WordBreakClass tcls = (QUnicodeTables::WordBreakClass) prop->wordBreakClass;

                if (Q_UNLIKELY(tcls == QUnicodeTables::WordBreak_Extend || tcls == QUnicodeTables::WordBreak_ZWJ || tcls == QUnicodeTables::WordBreak_Format)) {
                    // WB4: X(Extend|Format)* -> X
                    continue;
                }

                if (Q_LIKELY(tcls == cls || (action == WB::LookupW && (tcls == QUnicodeTables::WordBreak_HebrewLetter
                                                                       || tcls == QUnicodeTables::WordBreak_ALetter)))) {
                    i = lookahead;
                    ncls = tcls;
                    action = WB::NoBreak;
                }
                break;
            }
            if (action != WB::NoBreak) {
                action = WB::Break;
                if (Q_UNLIKELY(ncls == QUnicodeTables::WordBreak_SingleQuote && cls == QUnicodeTables::WordBreak_HebrewLetter))
                    action = WB::NoBreak; // WB7a
            }
            break;
        }

        cls = ncls;
        real_cls = ncls;

        if (action == WB::Break) {
            attributes[pos].wordBreak = true;
            if (currentWordType != WordTypeNone)
                attributes[pos].wordEnd = true;
            switch (cls) {
            case QUnicodeTables::WordBreak_Katakana:
                currentWordType = WordTypeHiraganaKatakana;
                attributes[pos].wordStart = true;
                break;
            case QUnicodeTables::WordBreak_HebrewLetter:
            case QUnicodeTables::WordBreak_ALetter:
            case QUnicodeTables::WordBreak_Numeric:
                currentWordType = WordTypeAlphaNumeric;
                attributes[pos].wordStart = true;
                break;
            default:
                currentWordType = WordTypeNone;
                break;
            }
        }
    }

    if (currentWordType != WordTypeNone)
        attributes[len].wordEnd = true;
    attributes[len].wordBreak = true; // WB2
}


namespace SB {

enum State {
    Initial,
    Lower,
    Upper,
    LUATerm,
    ATerm,
    ATermC,
    ACS,
    STerm,
    STermC,
    SCS,
    BAfterC,
    BAfter,
    Break,
    Lookup
};

static const uchar breakTable[BAfter + 1][QUnicodeTables::NumSentenceBreakClasses] = {
//    Any      CR       LF       Sep      Extend   Sp       Lower    Upper    OLetter  Numeric  ATerm   SContinue STerm    Close
    { Initial, BAfterC, BAfter , BAfter , Initial, Initial, Lower  , Upper  , Initial, Initial, ATerm  , Initial, STerm  , Initial }, // Initial
    { Initial, BAfterC, BAfter , BAfter , Lower  , Initial, Initial, Initial, Initial, Initial, LUATerm, Initial, STerm  , Initial }, // Lower
    { Initial, BAfterC, BAfter , BAfter , Upper  , Initial, Initial, Upper  , Initial, Initial, LUATerm, Initial, STerm  , Initial }, // Upper

    { Lookup , BAfterC, BAfter , BAfter , LUATerm, ACS    , Initial, Upper  , Break  , Initial, ATerm  , STerm  , STerm  , ATermC  }, // LUATerm
    { Lookup , BAfterC, BAfter , BAfter , ATerm  , ACS    , Initial, Break  , Break  , Initial, ATerm  , STerm  , STerm  , ATermC  }, // ATerm
    { Lookup , BAfterC, BAfter , BAfter , ATermC , ACS    , Initial, Break  , Break  , Lookup , ATerm  , STerm  , STerm  , ATermC  }, // ATermC
    { Lookup , BAfterC, BAfter , BAfter , ACS    , ACS    , Initial, Break  , Break  , Lookup , ATerm  , STerm  , STerm  , Lookup  }, // ACS

    { Break  , BAfterC, BAfter , BAfter , STerm  , SCS    , Break  , Break  , Break  , Break  , ATerm  , STerm  , STerm  , STermC  }, // STerm,
    { Break  , BAfterC, BAfter , BAfter , STermC , SCS    , Break  , Break  , Break  , Break  , ATerm  , STerm  , STerm  , STermC  }, // STermC
    { Break  , BAfterC, BAfter , BAfter , SCS    , SCS    , Break  , Break  , Break  , Break  , ATerm  , STerm  , STerm  , Break   }, // SCS
    { Break  , Break  , BAfter , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // BAfterC
    { Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // BAfter
};

} // namespace SB

static void getSentenceBreaks(const char16_t *string, qsizetype len, QCharAttributes *attributes)
{
    uchar state = SB::BAfter; // to meet SB1
    for (qsizetype i = 0; i != len; ++i) {
        qsizetype pos = i;
        char32_t ucs4 = string[i];
        if (QChar::isHighSurrogate(ucs4) && i + 1 != len) {
            ushort low = string[i + 1];
            if (QChar::isLowSurrogate(low)) {
                ucs4 = QChar::surrogateToUcs4(ucs4, low);
                ++i;
            }
        }

        const QUnicodeTables::Properties *prop = QUnicodeTables::properties(ucs4);
        QUnicodeTables::SentenceBreakClass ncls = (QUnicodeTables::SentenceBreakClass) prop->sentenceBreakClass;

        Q_ASSERT(state <= SB::BAfter);
        state = SB::breakTable[state][ncls];
        if (Q_UNLIKELY(state == SB::Lookup)) { // SB8
            state = SB::Break;
            for (qsizetype lookahead = i + 1; lookahead < len; ++lookahead) {
                ucs4 = string[lookahead];
                if (QChar::isHighSurrogate(ucs4) && lookahead + 1 != len) {
                    ushort low = string[lookahead + 1];
                    if (QChar::isLowSurrogate(low)) {
                        ucs4 = QChar::surrogateToUcs4(ucs4, low);
                        ++lookahead;
                    }
                }

                prop = QUnicodeTables::properties(ucs4);
                QUnicodeTables::SentenceBreakClass tcls = (QUnicodeTables::SentenceBreakClass) prop->sentenceBreakClass;
                switch (tcls) {
                case QUnicodeTables::SentenceBreak_Any:
                case QUnicodeTables::SentenceBreak_Extend:
                case QUnicodeTables::SentenceBreak_Sp:
                case QUnicodeTables::SentenceBreak_Numeric:
                case QUnicodeTables::SentenceBreak_SContinue:
                case QUnicodeTables::SentenceBreak_Close:
                    continue;
                case QUnicodeTables::SentenceBreak_Lower:
                    i = lookahead;
                    state = SB::Initial;
                    break;
                default:
                    break;
                }
                break;
            }
        }
        if (Q_UNLIKELY(state == SB::Break)) {
            attributes[pos].sentenceBoundary = true;
            state = SB::breakTable[SB::Initial][ncls];
        }
    }

    attributes[len].sentenceBoundary = true; // SB2
}


// -----------------------------------------------------------------------------------------------------
//
// The line breaking algorithm.
// See http://www.unicode.org/reports/tr14/tr14-39.html
//
// -----------------------------------------------------------------------------------------------------

namespace LB {

namespace NS { // Number Sequence

// This namespace is used to implement LB25 which, as of Unicode 16, has this
// definition:
// NU ( SY | IS )* CL × PO
// NU ( SY | IS )* CP × PO
// NU ( SY | IS )* CL × PR
// NU ( SY | IS )* CP × PR
// NU ( SY | IS )* × PO
// NU ( SY | IS )* × PR
// PO × OP NU
// PO × OP IS NU
// PO × NU
// PR × OP NU
// PR × OP IS NU
// PR × NU
// HY × NU
// IS × NU
// NU ( SY | IS )* × NU

enum Action {
    None,
    Start,
    Continue,
    Break,
    NeedOPNU, // Like Start, but must be followed by sequence `(OP (IS)?)? NU`
    // These are 'synthetic' actions and are not used in the table but are
    // tracked otherwise in the code for LB25, to track the state of specific
    // sequences:
    CNeedNU, // Like Continue, but must be followed by NU
    CNeedISNU, // Like Continue, but must be followed by IS? NU
};

enum Class {
    XX,
    PRPO,
    OP,
    HY,
    NU,
    SY,
    IS,
    CLCP
};

static const uchar actionTable[CLCP + 1][CLCP + 1] = {
//     XX       PRPO      OP        HY        NU        SY        IS        CLCP
    { None    , NeedOPNU, Start   , None    , Start   , None    , None    , None     }, // XX
    { None    , NeedOPNU, Continue, Break   , Start   , None    , None    , None     }, // PRPO
    { None    , Start   , Start   , Break   , Continue, None    , Continue, None     }, // OP
    { None    , None    , None    , Start   , Continue, None    , None    , None     }, // HY
    { Break   , Break   , Break   , Break   , Continue, Continue, Continue, Continue }, // NU
    { Break   , Break   , Break   , Break   , Continue, Continue, Continue, Continue }, // SY
    { Break   , Break   , Break   , Break   , Continue, Continue, Continue, Continue }, // IS
    { Break   , Continue, Break   , Break   , Break   , Break   , Break   , Break    }, // CLCP
};

inline Class toClass(QUnicodeTables::LineBreakClass lbc)
{
    switch (lbc) {
    case QUnicodeTables::LineBreak_PR: case QUnicodeTables::LineBreak_PO:
        return PRPO;
    case QUnicodeTables::LineBreak_OP:
        return OP;
    case QUnicodeTables::LineBreak_HY:
        return HY;
    case QUnicodeTables::LineBreak_NU:
        return NU;
    case QUnicodeTables::LineBreak_SY:
        return SY;
    case QUnicodeTables::LineBreak_IS:
        return IS;
    case QUnicodeTables::LineBreak_CL: case QUnicodeTables::LineBreak_CP:
        return CLCP;
    default:
        break;
    }
    return XX;
}

} // namespace NS

namespace BRS { // Brahmic Sequence, used to implement LB28a
    constexpr char32_t DottedCircle = U'\u25CC';

    // The LB28a_{n} value maps to the 'regex' on the nth line in LB28a
    // The only special case is LB28a_2VI which is a direct match to the 2nd
    // line, but it also leads to LB28a_3VIAK, the 3rd line.
    enum State {
        None,
        Start, // => Have: `(AK | [◌] | AS)`
        LB28a_2VF, // => Have: `(AK | [◌] | AS) VF`
        LB28a_2VI, // => Have: `(AK | [◌] | AS) VI` May find: `(AK | [◌])`
        LB28a_3VIAK, // => Have: `(AK | [◌] | AS) VI (AK | [◌])`
        LB28a_4, // => Have: `(AK | [◌] | AS) (AK | [◌] | AS)` May find: `VF`
        LB28a_4VF, // => Have: `(AK | [◌] | AS) (AK | [◌] | AS) VF`
        Restart,
    };
    struct LinebreakUnit {
        QUnicodeTables::LineBreakClass lbc;
        char32_t ucs4;
    };
    struct ParseState {
        State state = None;
        qsizetype start = 0;
    };
    State updateState(State state, LinebreakUnit lb)
    {
        using LBC = QUnicodeTables::LineBreakClass;
        if (lb.lbc == LBC::LineBreak_CM)
            return state;

        switch (state) {
        case Start:
            if (lb.lbc == LBC::LineBreak_VF)
                return LB28a_2VF;
            if (lb.lbc == LBC::LineBreak_VI)
                return LB28a_2VI;
            if (lb.ucs4 == DottedCircle || lb.lbc == LBC::LineBreak_AK
                || lb.lbc == LBC::LineBreak_AS)
                return LB28a_4;
            break;
        case LB28a_2VI:
            if (lb.ucs4 == DottedCircle || lb.lbc == LBC::LineBreak_AK)
                return LB28a_3VIAK;
            break;
        case LB28a_4:
            if (lb.lbc == LBC::LineBreak_VF)
                return LB28a_4VF;
            // Had (AK | [◌] | AS) (AK | [◌] | AS), which could mean the 2nd capture is the start
            // of a new sequence, so we need to check if it makes sense.
            return Restart;
        case None:
            if (Q_UNLIKELY(lb.ucs4 == DottedCircle || lb.lbc == LBC::LineBreak_AK
                           || lb.lbc == LBC::LineBreak_AS)) {
                return Start;
            }
            break;
        case LB28a_2VF:
        case LB28a_4VF:
        case LB28a_3VIAK:
        case Restart:
            // These are all terminal states, so no need to update
            Q_UNREACHABLE();
        }
        return None;
    }
}

enum Action {
    ProhibitedBreak, PB = ProhibitedBreak,
    DirectBreak, DB = DirectBreak,
    IndirectBreak, IB = IndirectBreak,
    CombiningIndirectBreak, CI = CombiningIndirectBreak,
    CombiningProhibitedBreak, CP = CombiningProhibitedBreak,
    ProhibitedBreakAfterHebrewPlusHyphen, HH = ProhibitedBreakAfterHebrewPlusHyphen,
    IndirectBreakIfNarrow, IN = IndirectBreakIfNarrow, // For LB30
    DirectBreakOutsideNumericSequence, DN = DirectBreakOutsideNumericSequence, // For LB25
};

// See https://www.unicode.org/reports/tr14/tr14-37.html for the information
// about the table. It was removed in the later versions of the standard.
static const uchar breakTable[QUnicodeTables::LineBreak_ZWJ][QUnicodeTables::LineBreak_ZWJ] = {
/* 1↓ 2→   OP  CL  CP  QU  +Pi +Pf +19 GL  NS  EX  SY  IS  PR  PO  NU  AL  HL  ID  IN  HY  +WS BA +WS HYBA BB  B2  ZW  CM  WJ  H2  H3  JL  JV  JT  RI  CB  EB  EM  AK  AP  AS  VI  VF*/
/* OP */ { PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, CP, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB },
/* CL */ { DB, PB, PB, IB, IB, PB, IB, IB, PB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* CP */ { DB, PB, PB, IB, IB, PB, IB, IB, PB, PB, PB, PB, DB, DB, IB, IB, IB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* QU */ { IB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, DB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB },
/* +Pi*/ { PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, CP, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB },
/* +Pf*/ { IB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB },
/* +19*/ { IB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB },
/* GL */ { IB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB },
/* NS */ { DB, PB, PB, DB, IB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* EX */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* SY */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, IB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* IS */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DN, DB, IB, IB, IB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* PR */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, DB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, IB, IB, IB, IB, IB, DB, DB, IB, IB, DB, DB, DB, DB, DB },
/* PO */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, DB, IB, IB, IB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* NU */ { IN, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* AL */ { IN, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* HL */ { IN, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, DB, IB, IB, CI, CI, CI, CI, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* ID */ { DB, PB, PB, DB, IB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* IN */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* HY */ { HH, PB, PB, IB, IB, PB, IB, HH, IB, PB, PB, PB, HH, HH, IB, HH, HH, HH, IB, IB, IB, IB, IB, IB, HH, HH, PB, CI, PB, HH, HH, HH, HH, HH, HH, DB, DB, DB, HH, HH, HH, HH, HH },
/* +WS*/ { HH, PB, PB, IB, IB, PB, IB, HH, IB, PB, PB, PB, HH, HH, IB, IB, HH, HH, IB, IB, IB, IB, IB, IB, HH, HH, PB, CI, PB, HH, HH, HH, HH, HH, HH, DB, DB, DB, HH, HH, HH, HH, HH },
/* BA */ { HH, PB, PB, IB, IB, PB, IB, HH, IB, PB, PB, PB, HH, HH, HH, HH, HH, HH, IB, IB, IB, IB, IB, IB, HH, HH, PB, CI, PB, HH, HH, HH, HH, HH, HH, DB, DB, DB, HH, HH, HH, HH, HH },
/* +WS*/ { HH, PB, PB, IB, IB, PB, IB, HH, IB, PB, PB, PB, HH, HH, HH, IB, HH, HH, IB, IB, IB, IB, IB, IB, HH, HH, PB, CI, PB, HH, HH, HH, HH, HH, HH, DB, DB, DB, HH, HH, HH, HH, HH },
/*HYBA*/ { PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, DB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB },
/* BB */ { IB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB, IB, DB, IB, IB, IB, IB, IB, IB, IB },
/* B2 */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, PB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* ZW */ { DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* CM */ { IB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, DB, IB, IB, IB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* WJ */ { IB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB },
/* H2 */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, IB, IB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* H3 */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, IB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* JL */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, IB, IB, IB, IB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* JV */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, IB, IB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* JT */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, IB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* RI */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, IB, DB, DB, DB, DB, DB, DB, DB, DB },
/* CB */ { DB, PB, PB, IB, IB, PB, IB, IB, DB, PB, PB, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* EB */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, IB, DB, DB, DB, DB, DB },
/* EM */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* AK */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, DB, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB, IB },
/* AP */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, DB, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB, DB, IB, DB, DB },
/* AS */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, DB, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, IB, IB },
/* VI */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, DB, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
/* VF */ { DB, PB, PB, IB, IB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, IB, IB, IB, IB, DB, DB, PB, DB, PB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB },
};

// The following line break classes are not treated by the pair table
// and must be resolved outside:
//  AI, BK, CB, CJ, CR, LF, NL, SA, SG, SP, XX, ZWJ

} // namespace LB

static void getLineBreaks(const char16_t *string, qsizetype len, QCharAttributes *attributes, QUnicodeTools::CharAttributeOptions options)
{
    qsizetype nestart = 0;
    LB::NS::Class nelast = LB::NS::XX;
    LB::NS::Action neactlast = LB::NS::None;

    LB::BRS::ParseState brsState;

    QUnicodeTables::LineBreakClass lcls = QUnicodeTables::LineBreak_LF; // to meet LB10
    QUnicodeTables::LineBreakClass cls = lcls;
    const QUnicodeTables::Properties *lastProp = QUnicodeTables::properties(U'\n');

    constexpr static auto isEastAsian = [](QUnicodeTables::EastAsianWidth eaw) {
        using EAW = QUnicodeTables::EastAsianWidth;
        return eaw == EAW::W || eaw == EAW::F || eaw == EAW::H;
    };

    for (qsizetype i = 0; i != len; ++i) {
        qsizetype pos = i;
        char32_t ucs4 = string[i];
        if (QChar::isHighSurrogate(ucs4) && i + 1 != len) {
            ushort low = string[i + 1];
            if (QChar::isLowSurrogate(low)) {
                ucs4 = QChar::surrogateToUcs4(ucs4, low);
                ++i;
            }
        }

        const QUnicodeTables::Properties *prop = QUnicodeTables::properties(ucs4);
        QUnicodeTables::LineBreakClass ncls = (QUnicodeTables::LineBreakClass) prop->lineBreakClass;
        QUnicodeTables::LineBreakClass tcls;

        if (options & QUnicodeTools::HangulLineBreakTailoring) {
            if (Q_UNLIKELY((ncls >= QUnicodeTables::LineBreak_H2
                        &&  ncls <= QUnicodeTables::LineBreak_JT)
                        || (ucs4 >= 0x3130 && ucs4 <= 0x318F && ncls == QUnicodeTables::LineBreak_ID))
                    ) {
                // LB27: use SPACE for line breaking
                // "When Korean uses SPACE for line breaking, the classes in rule LB26,
                // as well as characters of class ID, are often tailored to AL; see Section 8, Customization."
                // In case of Korean syllables: "3130..318F  HANGUL COMPATIBILITY JAMO"
                ncls = QUnicodeTables::LineBreak_AL;
            } else {
                if (Q_UNLIKELY(ncls == QUnicodeTables::LineBreak_SA)) {
                    // LB1: resolve SA to AL, except of those that have Category Mn or Mc be resolved to CM
                    static const int test = FLAG(QChar::Mark_NonSpacing) | FLAG(QChar::Mark_SpacingCombining);
                    if (FLAG(prop->category) & test)
                        ncls = QUnicodeTables::LineBreak_CM;
                }
            }
        }

        if (Q_UNLIKELY(ncls == QUnicodeTables::LineBreak_SA)) {
            // LB1: resolve SA to AL, except of those that have Category Mn or Mc be resolved to CM
            static const int test = FLAG(QChar::Mark_NonSpacing) | FLAG(QChar::Mark_SpacingCombining);
            if (FLAG(prop->category) & test)
                ncls = QUnicodeTables::LineBreak_CM;
        }

        if (Q_UNLIKELY(ncls == QUnicodeTables::LineBreak_QU)) {
            if (prop->category == QChar::Punctuation_InitialQuote) {
                // LB15a: Do not break after an unresolved initial punctuation
                // that lies at the start of the line, after a space, after
                // opening punctuation, or after an unresolved quotation mark,
                // even after spaces.
                //   (sot | BK | CR | LF | NL | OP | QU | GL | SP | ZW)
                //     [\p{Pi}&QU] SP* ×
                // Note: sot is treated as LF here due to initial loop setup.
                constexpr QUnicodeTables::LineBreakClass lb15a[] = {
                        QUnicodeTables::LineBreak_BK,    QUnicodeTables::LineBreak_CR,
                        QUnicodeTables::LineBreak_LF,    QUnicodeTables::LineBreak_OP,
                        QUnicodeTables::LineBreak_QU,    QUnicodeTables::LineBreak_QU_Pi,
                        QUnicodeTables::LineBreak_QU_Pf, QUnicodeTables::LineBreak_GL,
                        QUnicodeTables::LineBreak_SP,    QUnicodeTables::LineBreak_ZW};
                if (std::any_of(std::begin(lb15a), std::end(lb15a),
                                [lcls](auto x) { return x == lcls; })) {
                    ncls = QUnicodeTables::LineBreak_QU_Pi;
                }
            } else if (prop->category == QChar::Punctuation_FinalQuote) {
                // LB15b: Do not break before an unresolved final punctuation
                // that lies at the end of the line, before a space, before
                // a prohibited break, or before an unresolved quotation mark,
                // even after spaces.
                //   × [\p{Pf}&QU] ( SP | GL | WJ | CL | QU | CP | EX | IS
                //     | SY | BK | CR | LF | NL | ZW | eot)
                auto nncls = QUnicodeTables::LineBreak_LF;

                if (i + 1 < len) {
                    char32_t c = string[i + 1];
                    if (QChar::isHighSurrogate(c) && i + 2 < len) {
                        ushort low = string[i + 2];
                        if (QChar::isLowSurrogate(low))
                            c = QChar::surrogateToUcs4(c, low);
                    }
                    nncls = QUnicodeTables::LineBreakClass(
                            QUnicodeTables::properties(c)->lineBreakClass);
                }

                constexpr QUnicodeTables::LineBreakClass lb15b[] = {
                        QUnicodeTables::LineBreak_SP,    QUnicodeTables::LineBreak_GL,
                        QUnicodeTables::LineBreak_WJ,    QUnicodeTables::LineBreak_CL,
                        QUnicodeTables::LineBreak_QU,    QUnicodeTables::LineBreak_QU_Pi,
                        QUnicodeTables::LineBreak_QU_Pf, QUnicodeTables::LineBreak_CP,
                        QUnicodeTables::LineBreak_EX,    QUnicodeTables::LineBreak_IS,
                        QUnicodeTables::LineBreak_SY,    QUnicodeTables::LineBreak_BK,
                        QUnicodeTables::LineBreak_CR,    QUnicodeTables::LineBreak_LF,
                        QUnicodeTables::LineBreak_ZW};
                if (std::any_of(std::begin(lb15b), std::end(lb15b),
                                [nncls](auto x) { return x == nncls; })) {
                    ncls = QUnicodeTables::LineBreak_QU_Pf;
                }
            }
        }

        if (Q_UNLIKELY((lcls >= QUnicodeTables::LineBreak_SP || lcls == QUnicodeTables::LineBreak_ZW
                        || lcls == QUnicodeTables::LineBreak_GL
                        || lcls == QUnicodeTables::LineBreak_CB)
                       && (ncls == QUnicodeTables::LineBreak_HY || ucs4 == u'\u2010'))) {
            // LB20a: Do not break after a word-initial hyphen.
            // ( sot | BK | CR | LF | NL | SP | ZW | CB | GL ) ( HY | [\u2010] ) × AL

            // Remap to the synthetic class WS_* (whitespace+*), which is just
            // like the current respective linebreak class but with an IB action
            // if the next class is AL.
            if (ucs4 == u'\u2010')
                ncls = QUnicodeTables::LineBreak_WS_BA;
            else
                ncls = QUnicodeTables::LineBreak_WS_HY;
        }

        if (Q_UNLIKELY(cls == QUnicodeTables::LineBreak_AP && ucs4 == LB::BRS::DottedCircle)) {
            // LB28a: Do not break inside the orthographic syllables of Brahmic scripts
            // AP × (AK | [◌] | AS)
            // @note: AP × (AK | AS) is checked by the breakTable
            goto next;
        }
        while (true) { // May need to recheck once.
            // LB28a cont'd
            LB::BRS::State oldState = brsState.state;
            brsState.state = LB::BRS::updateState(brsState.state, {ncls, ucs4});
            if (Q_LIKELY(brsState.state == oldState))
                break;
            switch (brsState.state) {
            case LB::BRS::Start:
                brsState.start = i;
                break;
            case LB::BRS::LB28a_2VI: // Wait for more characters, but also valid sequence
                // We may get another character, but this is already a complete
                // sequence that should not have any breaks:
                for (qsizetype j = brsState.start + 1; j < i; ++j)
                    attributes[j].lineBreak = false;
                // No need to mark this sequence again later, so move 'start'
                // up to the current position:
                brsState.start = i;
                goto next;
            case LB::BRS::Restart:
                // The previous character was possibly the start of a new sequence
                brsState.state = LB::BRS::Start;
                brsState.start = pos - 1;
                continue; // Doing the loop again!
            case LB::BRS::LB28a_2VF:
            case LB::BRS::LB28a_4VF:
            case LB::BRS::LB28a_3VIAK:
                for (qsizetype j = brsState.start + 1; j < i; ++j)
                    attributes[j].lineBreak = false;
                if (brsState.state == LB::BRS::LB28a_3VIAK) {
                    // This might be the start of a new sequence
                    brsState.state = LB::BRS::Start;
                    brsState.start = i;
                } else {
                    brsState.state = LB::BRS::None;
                }
                goto next;
            case LB::BRS::LB28a_4: // Wait for more characters
            Q_LIKELY_BRANCH
            case LB::BRS::None: // Nothing to do
                break;
            }
            break;
        }

        if (Q_UNLIKELY(ncls == QUnicodeTables::LineBreak_IS)) {
            // LB15c Break before a decimal mark that follows a space, for instance, in
            // ‘subtract .5’.
            if (Q_UNLIKELY(lcls == QUnicodeTables::LineBreak_SP)) {
                if (i + 1 < len) {
                    char32_t ch = string[i + 1];
                    if (QChar::isHighSurrogate(ch) && i + 2 < len) {
                        ushort low = string[i + 2];
                        if (QChar::isLowSurrogate(low))
                            ch = QChar::surrogateToUcs4(ch, low);
                    }
                    if (QUnicodeTables::properties(ch)->lineBreakClass
                        == QUnicodeTables::LineBreak_NU) {
                        attributes[pos].lineBreak = true;
                        goto next;
                    }
                }
            }
        }

        if (Q_UNLIKELY(lcls == QUnicodeTables::LineBreak_HL)) {
            // LB21a Do not break after the hyphen in Hebrew + Hyphen + non-Hebrew
            // HL (HY | [ BA - $EastAsian ]) × [^HL]
            auto eaw = QUnicodeTables::EastAsianWidth(prop->eastAsianWidth);
            const bool isNonEaBA = ncls == QUnicodeTables::LineBreak_BA && !isEastAsian(eaw);
            if (isNonEaBA || ncls == QUnicodeTables::LineBreak_HY) {
                // Remap to synthetic HYBA class which handles the next
                // character. Generally (LB21) there are no breaks before
                // HY or BA, so we can skip ahead to the next character.
                ncls = QUnicodeTables::LineBreak_HYBA;
                goto next;
            }
        }

        // LB25: do not break lines inside numbers
        {
            LB::NS::Class necur = LB::NS::toClass(ncls);
            LB::NS::Action neact = LB::NS::Action(LB::NS::actionTable[nelast][necur]);
            if (Q_UNLIKELY(neactlast == LB::NS::CNeedNU && necur != LB::NS::NU)) {
                neact = LB::NS::None;
            } else if (Q_UNLIKELY(neactlast == LB::NS::NeedOPNU)) {
                if (necur == LB::NS::OP)
                    neact = LB::NS::CNeedISNU;
                else if (necur == LB::NS::NU)
                    neact = LB::NS::Continue;
                else // Anything else and we ignore the sequence
                    neact = LB::NS::None;
            } else if (Q_UNLIKELY(neactlast == LB::NS::CNeedISNU)) {
                if (necur == LB::NS::IS)
                    neact = LB::NS::CNeedNU;
                else if (necur == LB::NS::NU)
                    neact = LB::NS::Continue;
                else // Anything else and we ignore the sequence
                    neact = LB::NS::None;
            }
            switch (neact) {
            case LB::NS::Break:
                // do not change breaks before and after the expression
                for (qsizetype j = nestart + 1; j < pos; ++j)
                    attributes[j].lineBreak = false;
                Q_FALLTHROUGH();
            Q_LIKELY_BRANCH
            case LB::NS::None:
                nelast = LB::NS::XX; // reset state
                break;
            case LB::NS::NeedOPNU:
            case LB::NS::Start:
                if (neactlast == LB::NS::Start || neactlast == LB::NS::Continue) {
                    // Apply the linebreaks for the previous stretch; we need to start a new one
                    for (qsizetype j = nestart + 1; j < pos; ++j)
                        attributes[j].lineBreak = false;
                }
                nestart = i;
                Q_FALLTHROUGH();
            case LB::NS::CNeedNU:
            case LB::NS::CNeedISNU:
            case LB::NS::Continue:
                nelast = necur;
                break;
            }
            neactlast = neact;
        }

        // LB19a Unless surrounded by East Asian characters, do not break either side of any
        // unresolved quotation marks
        if (Q_UNLIKELY(ncls == QUnicodeTables::LineBreak_QU
                       && lcls != QUnicodeTables::LineBreak_SP
                       && lcls != QUnicodeTables::LineBreak_ZW)) {
            using EAW = QUnicodeTables::EastAsianWidth;
            constexpr static auto nextCharNonEastAsian = [](const char16_t *string, qsizetype len) {
                if (len > 0) {
                    char32_t nch = string[0];
                    if (QChar::isHighSurrogate(nch) && len > 1) {
                        char16_t low = string[1];
                        if (QChar::isLowSurrogate(low))
                            nch = QChar::surrogateToUcs4(char16_t(nch), low);
                    }
                    const auto *nextProp = QUnicodeTables::properties(nch);
                    QUnicodeTables::LineBreakClass nncls = QUnicodeTables::LineBreakClass(
                            nextProp->lineBreakClass);
                    QUnicodeTables::EastAsianWidth neaw = EAW(nextProp->eastAsianWidth);
                    return nncls != QUnicodeTables::LineBreak_CM
                            && nncls <= QUnicodeTables::LineBreak_SP
                            && !isEastAsian(neaw);
                }
                return true; // end-of-text counts as non-East-Asian
            };
            if (Q_UNLIKELY(!isEastAsian(EAW(lastProp->eastAsianWidth))
                           || nextCharNonEastAsian(string + i + 1, len - i - 1))) {
                // Remap to the synthetic QU_19 class which has indirect breaks
                // for most following classes.
                ncls = QUnicodeTables::LineBreak_QU_19;
            }
        }

        if (Q_UNLIKELY(lcls >= QUnicodeTables::LineBreak_CR)) {
            // LB4: BK!, LB5: (CRxLF|CR|LF|NL)!
            if (lcls > QUnicodeTables::LineBreak_CR || ncls != QUnicodeTables::LineBreak_LF)
                attributes[pos].lineBreak = attributes[pos].mandatoryBreak = true;
            goto next;
        }

        if (Q_UNLIKELY(ncls >= QUnicodeTables::LineBreak_SP)) {
            if (ncls > QUnicodeTables::LineBreak_SP)
                goto next; // LB6: x(BK|CR|LF|NL)
            goto next_no_cls_update; // LB7: xSP
        }

        // LB19 - do not break before non-initial unresolved quotation marks, or after non-final
        // unresolved quotation marks
        if (Q_UNLIKELY(((ncls == QUnicodeTables::LineBreak_QU
                         || ncls == QUnicodeTables::LineBreak_QU_19)
                        && prop->category != QChar::Punctuation_InitialQuote)
                       || (cls == QUnicodeTables::LineBreak_QU
                           && lastProp->category != QChar::Punctuation_FinalQuote))) {
            // Make sure the previous character is not one that we have to break after.
            // Also skip if ncls is CM so it can be treated as lcls (LB9)
            if (lcls != QUnicodeTables::LineBreak_SP && lcls != QUnicodeTables::LineBreak_ZW
                && ncls != QUnicodeTables::LineBreak_CM) {
                goto next;
            }
        }

        if (Q_UNLIKELY(ncls == QUnicodeTables::LineBreak_CM || ncls == QUnicodeTables::LineBreak_ZWJ)) {
            // LB9: treat CM that don't follows SP, BK, CR, LF, NL, or ZW as X
            if (lcls != QUnicodeTables::LineBreak_ZW && lcls < QUnicodeTables::LineBreak_SP)
                // don't update anything
                goto next_no_cls_update;
        }

        if (Q_UNLIKELY(lcls == QUnicodeTables::LineBreak_ZWJ)) {
            // LB8a: ZWJ x
            goto next;
        }

        if (Q_UNLIKELY(ncls == QUnicodeTables::LineBreak_RI && lcls == QUnicodeTables::LineBreak_RI)) {
            // LB30a
            ncls = QUnicodeTables::LineBreak_SP;
            goto next;
        }

        if (Q_UNLIKELY(ncls == QUnicodeTables::LineBreak_EM
                       && lastProp->category == QChar::Other_NotAssigned
                       && lastProp->graphemeBreakClass
                               == QUnicodeTables::GraphemeBreak_Extended_Pictographic)) {
            // LB30b: [\p{Extended_Pictographic}&\p{Cn}] × EM
            goto next;
        }

        // for South East Asian chars that require a complex analysis, the Unicode
        // standard recommends to treat them as AL. tailoring that do dictionary analysis can override
        if (Q_UNLIKELY(cls >= QUnicodeTables::LineBreak_SA))
            cls = QUnicodeTables::LineBreak_AL;

        tcls = cls;

        constexpr static auto remapToAL = [](QUnicodeTables::LineBreakClass &c, auto &property) {
            if (Q_UNLIKELY(c == QUnicodeTables::LineBreak_CM
                           || c == QUnicodeTables::LineBreak_ZWJ)) {
                c = QUnicodeTables::LineBreak_AL;
                property = QUnicodeTables::properties(U'\u0041');
            }
        };
        // LB10 Treat any remaining combining mark or ZWJ as AL,
        // as if it had the properties of U+0041 A LATIN CAPITAL LETTER
        remapToAL(tcls, lastProp);
        remapToAL(ncls, prop);

        switch (LB::breakTable[tcls][ncls < QUnicodeTables::LineBreak_ZWJ ? ncls : QUnicodeTables::LineBreak_AL]) {
        case LB::DirectBreak:
            attributes[pos].lineBreak = true;
            break;
        case LB::IndirectBreak:
            if (lcls == QUnicodeTables::LineBreak_SP)
                attributes[pos].lineBreak = true;
            break;
        case LB::CombiningIndirectBreak:
            if (lcls != QUnicodeTables::LineBreak_SP)
                goto next_no_cls_update;
            attributes[pos].lineBreak = true;
            break;
        case LB::CombiningProhibitedBreak:
            if (lcls != QUnicodeTables::LineBreak_SP)
                goto next_no_cls_update;
            break;
        case LB::ProhibitedBreakAfterHebrewPlusHyphen:
            if (lcls != QUnicodeTables::LineBreak_HL)
                attributes[pos].lineBreak = true;
            break;
        case LB::IndirectBreakIfNarrow:
            using EAW = QUnicodeTables::EastAsianWidth;
            switch (EAW(prop->eastAsianWidth)) {
            default:
                if (lcls != QUnicodeTables::LineBreak_SP)
                    break;
                Q_FALLTHROUGH();
            case QUnicodeTables::EastAsianWidth::F:
            case QUnicodeTables::EastAsianWidth::W:
            case QUnicodeTables::EastAsianWidth::H:
                attributes[pos].lineBreak = true;
                break;
            }
            break;
        case LB::DirectBreakOutsideNumericSequence:
            if (neactlast == LB::NS::None || neactlast > LB::NS::Break)
                attributes[pos].lineBreak = true;
            break;
        case LB::ProhibitedBreak:
            // nothing to do
        default:
            break;
        }

    next:
        if (ncls != QUnicodeTables::LineBreak_CM && ncls != QUnicodeTables::LineBreak_ZWJ) {
            cls = ncls;
            lastProp = prop;
        }
    next_no_cls_update:
        lcls = ncls;
    }

    if (Q_UNLIKELY(LB::NS::actionTable[nelast][LB::NS::XX] == LB::NS::Break)) {
        // LB25: do not break lines inside numbers
        for (qsizetype j = nestart + 1; j < len; ++j)
            attributes[j].lineBreak = false;
    }

    attributes[0].lineBreak = attributes[0].mandatoryBreak = false; // LB2
    attributes[len].lineBreak = attributes[len].mandatoryBreak = true; // LB3
}


static void getWhiteSpaces(const char16_t *string, qsizetype len, QCharAttributes *attributes)
{
    for (qsizetype i = 0; i != len; ++i) {
        uint ucs4 = string[i];
        if (QChar::isHighSurrogate(ucs4) && i + 1 != len) {
            ushort low = string[i + 1];
            if (QChar::isLowSurrogate(low)) {
                ucs4 = QChar::surrogateToUcs4(ucs4, low);
                ++i;
            }
        }

        if (Q_UNLIKELY(QChar::isSpace(ucs4)))
            attributes[i].whiteSpace = true;
    }
}

namespace Tailored {

using CharAttributeFunction = void (*)(QChar::Script script, const char16_t *text, qsizetype from, qsizetype len, QCharAttributes *attributes);


enum Form {
    Invalid = 0x0,
    UnknownForm = Invalid,
    Consonant,
    Nukta,
    Halant,
    Matra,
    VowelMark,
    StressMark,
    IndependentVowel,
    LengthMark,
    Control,
    Other
};

static const unsigned char indicForms[0xe00-0x900] = {
    // Devangari
    Invalid, VowelMark, VowelMark, VowelMark,
    IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,

    IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, UnknownForm, UnknownForm,
    Nukta, Other, Matra, Matra,

    Matra, Matra, Matra, Matra,
    Matra, Matra, Matra, Matra,
    Matra, Matra, Matra, Matra,
    Matra, Halant, UnknownForm, UnknownForm,

    Other, StressMark, StressMark, StressMark,
    StressMark, UnknownForm, UnknownForm, UnknownForm,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    IndependentVowel, IndependentVowel, VowelMark, VowelMark,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Consonant,
    Consonant, Consonant /* ??? */, Consonant, Consonant,

    // Bengali
    Invalid, VowelMark, VowelMark, VowelMark,
    Invalid, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, Invalid, Invalid, IndependentVowel,

    IndependentVowel, Invalid, Invalid, IndependentVowel,
    IndependentVowel, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Invalid, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Invalid, Consonant, Invalid,
    Invalid, Invalid, Consonant, Consonant,
    Consonant, Consonant, UnknownForm, UnknownForm,
    Nukta, Other, Matra, Matra,

    Matra, Matra, Matra, Matra,
    Matra, Invalid, Invalid, Matra,
    Matra, Invalid, Invalid, Matra,
    Matra, Halant, Consonant, UnknownForm,

    Invalid, Invalid, Invalid, Invalid,
    Invalid, Invalid, Invalid, VowelMark,
    Invalid, Invalid, Invalid, Invalid,
    Consonant, Consonant, Invalid, Consonant,

    IndependentVowel, IndependentVowel, VowelMark, VowelMark,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    Consonant, Consonant, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    // Gurmukhi
    Invalid, VowelMark, VowelMark, VowelMark,
    Invalid, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, IndependentVowel, Invalid,
    Invalid, Invalid, Invalid, IndependentVowel,

    IndependentVowel, Invalid, Invalid, IndependentVowel,
    IndependentVowel, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Invalid, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Invalid, Consonant, Consonant,
    Invalid, Consonant, Consonant, Invalid,
    Consonant, Consonant, UnknownForm, UnknownForm,
    Nukta, Other, Matra, Matra,

    Matra, Matra, Matra, Invalid,
    Invalid, Invalid, Invalid, Matra,
    Matra, Invalid, Invalid, Matra,
    Matra, Halant, UnknownForm, UnknownForm,

    Invalid, Invalid, Invalid, Invalid,
    Invalid, UnknownForm, UnknownForm, UnknownForm,
    Invalid, Consonant, Consonant, Consonant,
    Consonant, Invalid, Consonant, Invalid,

    Other, Other, Invalid, Invalid,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    StressMark, StressMark, Consonant, Consonant,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    // Gujarati
    Invalid, VowelMark, VowelMark, VowelMark,
    Invalid, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, Invalid, IndependentVowel,

    IndependentVowel, IndependentVowel, Invalid, IndependentVowel,
    IndependentVowel, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Invalid, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Invalid, Consonant, Consonant,
    Invalid, Consonant, Consonant, Consonant,
    Consonant, Consonant, UnknownForm, UnknownForm,
    Nukta, Other, Matra, Matra,

    Matra, Matra, Matra, Matra,
    Matra, Matra, Invalid, Matra,
    Matra, Matra, Invalid, Matra,
    Matra, Halant, UnknownForm, UnknownForm,

    Other, UnknownForm, UnknownForm, UnknownForm,
    UnknownForm, UnknownForm, UnknownForm, UnknownForm,
    UnknownForm, UnknownForm, UnknownForm, UnknownForm,
    UnknownForm, UnknownForm, UnknownForm, UnknownForm,

    IndependentVowel, IndependentVowel, VowelMark, VowelMark,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    // Oriya
    Invalid, VowelMark, VowelMark, VowelMark,
    Invalid, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, Invalid, Invalid, IndependentVowel,

    IndependentVowel, Invalid, Invalid, IndependentVowel,
    IndependentVowel, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Invalid, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Invalid, Consonant, Consonant,
    Invalid, Consonant, Consonant, Consonant,
    Consonant, Consonant, UnknownForm, UnknownForm,
    Nukta, Other, Matra, Matra,

    Matra, Matra, Matra, Matra,
    Invalid, Invalid, Invalid, Matra,
    Matra, Invalid, Invalid, Matra,
    Matra, Halant, UnknownForm, UnknownForm,

    Other, Invalid, Invalid, Invalid,
    Invalid, UnknownForm, LengthMark, LengthMark,
    Invalid, Invalid, Invalid, Invalid,
    Consonant, Consonant, Invalid, Consonant,

    IndependentVowel, IndependentVowel, Invalid, Invalid,
    Invalid, Invalid, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    Other, Consonant, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    //Tamil
    Invalid, Invalid, VowelMark, Other,
    Invalid, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, IndependentVowel, Invalid,
    Invalid, Invalid, IndependentVowel, IndependentVowel,

    IndependentVowel, Invalid, IndependentVowel, IndependentVowel,
    IndependentVowel, Consonant, Invalid, Invalid,
    Invalid, Consonant, Consonant, Invalid,
    Consonant, Invalid, Consonant, Consonant,

    Invalid, Invalid, Invalid, Consonant,
    Consonant, Invalid, Invalid, Invalid,
    Consonant, Consonant, Consonant, Invalid,
    Invalid, Invalid, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, UnknownForm, UnknownForm,
    Invalid, Invalid, Matra, Matra,

    Matra, Matra, Matra, Invalid,
    Invalid, Invalid, Matra, Matra,
    Matra, Invalid, Matra, Matra,
    Matra, Halant, Invalid, Invalid,

    Invalid, Invalid, Invalid, Invalid,
    Invalid, Invalid, Invalid, LengthMark,
    Invalid, Invalid, Invalid, Invalid,
    Invalid, Invalid, Invalid, Invalid,

    Invalid, Invalid, Invalid, Invalid,
    Invalid, Invalid, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    // Telugu
    Invalid, VowelMark, VowelMark, VowelMark,
    Invalid, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, Invalid, IndependentVowel, IndependentVowel,

    IndependentVowel, Invalid, IndependentVowel, IndependentVowel,
    IndependentVowel, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Invalid, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Invalid, Consonant, Consonant, Consonant,
    Consonant, Consonant, UnknownForm, UnknownForm,
    Invalid, Invalid, Matra, Matra,

    Matra, Matra, Matra, Matra,
    Matra, Invalid, Matra, Matra,
    Matra, Invalid, Matra, Matra,
    Matra, Halant, Invalid, Invalid,

    Invalid, Invalid, Invalid, Invalid,
    Invalid, LengthMark, Matra, Invalid,
    Invalid, Invalid, Invalid, Invalid,
    Invalid, Invalid, Invalid, Invalid,

    IndependentVowel, IndependentVowel, Invalid, Invalid,
    Invalid, Invalid, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    // Kannada
    Invalid, Invalid, VowelMark, VowelMark,
    Invalid, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, Invalid, IndependentVowel, IndependentVowel,

    IndependentVowel, Invalid, IndependentVowel, IndependentVowel,
    IndependentVowel, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Invalid, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Invalid, Consonant, Consonant, Consonant,
    Consonant, Consonant, UnknownForm, UnknownForm,
    Nukta, Other, Matra, Matra,

    Matra, Matra, Matra, Matra,
    Matra, Invalid, Matra, Matra,
    Matra, Invalid, Matra, Matra,
    Matra, Halant, Invalid, Invalid,

    Invalid, Invalid, Invalid, Invalid,
    Invalid, LengthMark, LengthMark, Invalid,
    Invalid, Invalid, Invalid, Invalid,
    Invalid, Invalid, Consonant, Invalid,

    IndependentVowel, IndependentVowel, VowelMark, VowelMark,
    Invalid, Invalid, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    // Malayalam
    Invalid, Invalid, VowelMark, VowelMark,
    Invalid, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, Invalid, IndependentVowel, IndependentVowel,

    IndependentVowel, Invalid, IndependentVowel, IndependentVowel,
    IndependentVowel, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Invalid, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, UnknownForm, UnknownForm,
    Invalid, Invalid, Matra, Matra,

    Matra, Matra, Matra, Matra,
    Invalid, Invalid, Matra, Matra,
    Matra, Invalid, Matra, Matra,
    Matra, Halant, Invalid, Invalid,

    Invalid, Invalid, Invalid, Invalid,
    Invalid, Invalid, Invalid, Matra,
    Invalid, Invalid, Invalid, Invalid,
    Invalid, Invalid, Invalid, Invalid,

    IndependentVowel, IndependentVowel, Invalid, Invalid,
    Invalid, Invalid, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,

    // Sinhala
    Invalid, Invalid, VowelMark, VowelMark,
    Invalid, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,

    IndependentVowel, IndependentVowel, IndependentVowel, IndependentVowel,
    IndependentVowel, IndependentVowel, IndependentVowel, Invalid,
    Invalid, Invalid, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,

    Consonant, Consonant, Invalid, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Consonant,
    Invalid, Consonant, Invalid, Invalid,

    Consonant, Consonant, Consonant, Consonant,
    Consonant, Consonant, Consonant, Invalid,
    Invalid, Invalid, Halant, Invalid,
    Invalid, Invalid, Invalid, Matra,

    Matra, Matra, Matra, Matra,
    Matra, Invalid, Matra, Invalid,
    Matra, Matra, Matra, Matra,
    Matra, Matra, Matra, Matra,

    Invalid, Invalid, Invalid, Invalid,
    Invalid, Invalid, Invalid, Invalid,
    Invalid, Invalid, Invalid, Invalid,
    Invalid, Invalid, Invalid, Invalid,

    Invalid, Invalid, Matra, Matra,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
    Other, Other, Other, Other,
};

static inline Form form(unsigned short uc) {
    if (uc < 0x900 || uc > 0xdff) {
        if (uc == 0x25cc)
            return Consonant;
        if (uc == 0x200c || uc == 0x200d)
            return Control;
        return Other;
    }
    return (Form)indicForms[uc-0x900];
}

// #define INDIC_DEBUG
#ifdef INDIC_DEBUG
#define IDEBUG qDebug
#else
#define IDEBUG if constexpr (1) ; else qDebug
#endif

/* syllables are of the form:

   (Consonant Nukta? Halant)* Consonant Matra? VowelMark? StressMark?
   (Consonant Nukta? Halant)* Consonant Halant
   IndependentVowel VowelMark? StressMark?

   We return syllable boundaries on invalid combinations as well
*/
static qsizetype indic_nextSyllableBoundary(QChar::Script script, const char16_t *s, qsizetype start, qsizetype end, bool *invalid)
{
    *invalid = false;
    IDEBUG("indic_nextSyllableBoundary: start=%lld, end=%lld", qlonglong(start), qlonglong(end));
    const char16_t *uc = s+start;

    qsizetype pos = 0;
    Form state = form(uc[pos]);
    IDEBUG("state[%lld]=%d (uc=%4x)", qlonglong(pos), state, uc[pos]);
    pos++;

    if (state != Consonant && state != IndependentVowel) {
        if (state != Other)
            *invalid = true;
        goto finish;
    }

    while (pos < end - start) {
        Form newState = form(uc[pos]);
        IDEBUG("state[%lld]=%d (uc=%4x)", qlonglong(pos), newState, uc[pos]);
        switch (newState) {
        case Control:
            newState = state;
        if (state == Halant && uc[pos] == 0x200d /* ZWJ */)
        break;
            // the control character should be the last char in the item
        if (state == Consonant && script == QChar::Script_Bengali && uc[pos-1] == 0x09B0 && uc[pos] == 0x200d /* ZWJ */)
        break;
        if (state == Consonant && script == QChar::Script_Kannada && uc[pos-1] == 0x0CB0 && uc[pos] == 0x200d /* ZWJ */)
        break;
            // Bengali and Kannada has a special exception for rendering yaphala with ra (to avoid reph) see http://www.unicode.org/faq/indic.html#15
            ++pos;
            goto finish;
        case Consonant:
        if (state == Halant && (script != QChar::Script_Sinhala || uc[pos-1] == 0x200d /* ZWJ */))
                break;
            goto finish;
        case Halant:
            if (state == Nukta || state == Consonant)
                break;
            // Bengali has a special exception allowing the combination Vowel_A/E + Halant + Ya
            if (script == QChar::Script_Bengali && pos == 1 &&
                 (uc[0] == 0x0985 || uc[0] == 0x098f))
                break;
            // Sinhala uses the Halant as a component of certain matras. Allow these, but keep the state on Matra.
            if (script == QChar::Script_Sinhala && state == Matra) {
                ++pos;
                continue;
            }
            if (script == QChar::Script_Malayalam && state == Matra && uc[pos-1] == 0x0d41) {
                ++pos;
                continue;
            }
            goto finish;
        case Nukta:
            if (state == Consonant)
                break;
            goto finish;
        case StressMark:
            if (state == VowelMark)
                break;
            Q_FALLTHROUGH();
        case VowelMark:
            if (state == Matra || state == LengthMark || state == IndependentVowel)
                break;
            Q_FALLTHROUGH();
        case Matra:
            if (state == Consonant || state == Nukta)
                break;
            if (state == Matra) {
                // ### needs proper testing for correct two/three part matras
                break;
            }
            // ### not sure if this is correct. If it is, does it apply only to Bengali or should
            // it work for all Indic languages?
            // the combination Independent_A + Vowel Sign AA is allowed.
            if (script == QChar::Script_Bengali && uc[pos] == 0x9be && uc[pos-1] == 0x985)
                break;
            if (script == QChar::Script_Tamil && state == Matra) {
                if (uc[pos-1] == 0x0bc6 &&
                     (uc[pos] == 0xbbe || uc[pos] == 0xbd7))
                    break;
                if (uc[pos-1] == 0x0bc7 && uc[pos] == 0xbbe)
                    break;
            }
            goto finish;

        case LengthMark:
            if (state == Matra) {
                // ### needs proper testing for correct two/three part matras
                break;
            }
            Q_FALLTHROUGH();
        case IndependentVowel:
        case Invalid:
        case Other:
            goto finish;
        }
        state = newState;
        pos++;
    }
 finish:
    return pos+start;
}

static void indicAttributes(QChar::Script script, const char16_t *text, qsizetype from, qsizetype len, QCharAttributes *attributes)
{
    qsizetype end = from + len;
    attributes += from;
    qsizetype i = 0;
    while (i < len) {
        bool invalid;
        qsizetype boundary = indic_nextSyllableBoundary(script, text, from+i, end, &invalid) - from;
         attributes[i].graphemeBoundary = true;

        if (boundary > len-1) boundary = len;
        i++;
        while (i < boundary) {
            attributes[i].graphemeBoundary = false;
            ++i;
        }
        assert(i == boundary);
    }


}

#if QT_CONFIG(library)

#define LIBTHAI_MAJOR   0

/*
 * if libthai changed please update these codes too.
 */
struct thcell_t {
    unsigned char base;      /**< base character */
    unsigned char hilo;      /**< upper/lower vowel/diacritic */
    unsigned char top;       /**< top-level mark */
};

using ThBrk = struct _ThBrk;

namespace {

class LibThai final
{
    Q_DISABLE_COPY_MOVE(LibThai)

    using th_brk_new_def = ThBrk *(*)(const char *);
    using th_brk_delete_def = void (*)(ThBrk *);
    using th_brk_find_breaks_def = int (*)(ThBrk *, const unsigned char *, int *, size_t);
    using th_next_cell_def = size_t (*)(const unsigned char *, size_t, struct thcell_t *, int);

public:
    LibThai() : m_library("thai"_L1, LIBTHAI_MAJOR)
    {
        m_th_brk_find_breaks =
                reinterpret_cast<th_brk_find_breaks_def>(m_library.resolve("th_brk_find_breaks"));
        m_th_next_cell = reinterpret_cast<th_next_cell_def>(m_library.resolve("th_next_cell"));

        auto th_brk_new = reinterpret_cast<th_brk_new_def>(m_library.resolve("th_brk_new"));
        if (th_brk_new) {
            m_state = th_brk_new(nullptr);
            m_th_brk_delete =
                    reinterpret_cast<th_brk_delete_def>(m_library.resolve("th_brk_delete"));
        }
    }

    ~LibThai()
    {
        if (m_state && m_th_brk_delete)
            m_th_brk_delete(m_state);
        m_library.unload();
    }

    bool isInitialized() const { return m_th_brk_find_breaks && m_th_next_cell && m_state; }

    int brk_find_breaks(const unsigned char *s, int *pos, size_t pos_sz) const
    {
        Q_ASSERT(m_state);
        Q_ASSERT(m_th_brk_find_breaks);
        return m_th_brk_find_breaks(m_state, s, pos, pos_sz);
    }

    size_t next_cell(const unsigned char *s, size_t len, struct thcell_t *cell, int is_decomp_am)
    {
        Q_ASSERT(m_th_next_cell);
        return m_th_next_cell(s, len, cell, is_decomp_am);
    }

private:
    QLibrary m_library;

    // Global state for th_brk_find_breaks().
    // Note: even if signature for th_brk_find_breaks() suggests otherwise, the
    // state is read-only, and so it is safe to use it from multiple threads after
    // initialization. This is also stated in the libthai documentation.
    ThBrk *m_state = nullptr;

    th_brk_find_breaks_def m_th_brk_find_breaks = nullptr;
    th_next_cell_def m_th_next_cell = nullptr;
    th_brk_delete_def m_th_brk_delete = nullptr;
};

} // unnamed namespace

Q_GLOBAL_STATIC(LibThai, g_libThai)

static void to_tis620(const char16_t *string, qsizetype len, char *cstr)
{
    qsizetype i;
    unsigned char *result = reinterpret_cast<unsigned char *>(cstr);

    for (i = 0; i < len; ++i) {
        if (string[i] <= 0xa0)
            result[i] = static_cast<unsigned char>(string[i]);
        else if (string[i] >= 0xe01 && string[i] <= 0xe5b)
            result[i] = static_cast<unsigned char>(string[i] - 0xe00 + 0xa0);
        else
            result[i] = static_cast<unsigned char>(~0); // Same encoding as libthai uses for invalid chars
    }

    result[len] = 0;
}

/*
 * Thai Attributes: computes Word Break, Word Boundary and Char stop for THAI.
 */
static void thaiAssignAttributes(const char16_t *string, qsizetype len, QCharAttributes *attributes)
{
    constexpr qsizetype Prealloc = 128;
    QVarLengthArray<char, Prealloc + 1> s(len + 1);
    QVarLengthArray<int, Prealloc> break_positions(len);
    qsizetype numbreaks, i;
    struct thcell_t tis_cell;

    LibThai *libThai = g_libThai;
    if (!libThai || !libThai->isInitialized())
        return;

    to_tis620(string, len, s.data());

    for (i = 0; i < len; ++i) {
        attributes[i].wordBreak = false;
        attributes[i].wordStart = false;
        attributes[i].wordEnd = false;
        attributes[i].lineBreak = false;
    }

    attributes[0].wordBreak = true;
    attributes[0].wordStart = true;
    attributes[0].wordEnd = false;
    numbreaks = libThai->brk_find_breaks(reinterpret_cast<const unsigned char *>(s.data()),
                                         break_positions.data(),
                                         static_cast<size_t>(break_positions.size()));
    for (i = 0; i < numbreaks; ++i) {
        attributes[break_positions[i]].wordBreak = true;
        attributes[break_positions[i]].wordStart = true;
        attributes[break_positions[i]].wordEnd = true;
        attributes[break_positions[i]].lineBreak = true;
    }
    if (numbreaks > 0)
        attributes[break_positions[numbreaks - 1]].wordStart = false;

    /* manage grapheme boundaries */
    i = 0;
    while (i < len) {
        size_t cell_length =
                libThai->next_cell(reinterpret_cast<const unsigned char *>(s.data()) + i,
                                   size_t(len - i), &tis_cell, true);

        attributes[i].graphemeBoundary = true;
        for (size_t j = 1; j < cell_length; ++j)
            attributes[i + j].graphemeBoundary = false;

        i += cell_length;
    }
}

#endif // QT_CONFIG(library)

static void thaiAttributes(QChar::Script script, const char16_t *text, qsizetype from, qsizetype len, QCharAttributes *attributes)
{
    assert(script == QChar::Script_Thai);
#if QT_CONFIG(library)
    const char16_t *uc = text + from;
    attributes += from;
    Q_UNUSED(script);
    thaiAssignAttributes(uc, len, attributes);
#else
    Q_UNUSED(script);
    Q_UNUSED(text);
    Q_UNUSED(from);
    Q_UNUSED(len);
    Q_UNUSED(attributes);
#endif
}

/*
 tibetan syllables are of the form:
    head position consonant
    first sub-joined consonant
    ....intermediate sub-joined consonants (if any)
    last sub-joined consonant
    sub-joined vowel (a-chung U+0F71)
    standard or compound vowel sign (or 'virama' for devanagari transliteration)
*/

typedef enum {
    TibetanOther,
    TibetanHeadConsonant,
    TibetanSubjoinedConsonant,
    TibetanSubjoinedVowel,
    TibetanVowel
} TibetanForm;

/* this table starts at U+0f40 */
static const unsigned char tibetanForm[0x80] = {
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,

    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,

    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant, TibetanHeadConsonant,
    TibetanOther, TibetanOther, TibetanOther, TibetanOther,

    TibetanOther, TibetanVowel, TibetanVowel, TibetanVowel,
    TibetanVowel, TibetanVowel, TibetanVowel, TibetanVowel,
    TibetanVowel, TibetanVowel, TibetanVowel, TibetanVowel,
    TibetanVowel, TibetanVowel, TibetanVowel, TibetanVowel,

    TibetanVowel, TibetanVowel, TibetanVowel, TibetanVowel,
    TibetanVowel, TibetanVowel, TibetanVowel, TibetanVowel,
    TibetanOther, TibetanOther, TibetanOther, TibetanOther,
    TibetanOther, TibetanOther, TibetanOther, TibetanOther,

    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,

    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,

    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant, TibetanSubjoinedConsonant,
    TibetanSubjoinedConsonant, TibetanOther, TibetanOther, TibetanOther
};

#define tibetan_form(c) \
    ((c) >= 0x0f40 && (c) < 0x0fc0 ? (TibetanForm)tibetanForm[(c) - 0x0f40] : TibetanOther)

static qsizetype tibetan_nextSyllableBoundary(const char16_t *s, qsizetype start, qsizetype end, bool *invalid)
{
    const char16_t *uc = s + start;

    qsizetype pos = 0;
    TibetanForm state = tibetan_form(*uc);

/*     qDebug("state[%d]=%d (uc=%4x)", pos, state, uc[pos]);*/
    pos++;

    if (state != TibetanHeadConsonant) {
        if (state != TibetanOther)
            *invalid = true;
        goto finish;
    }

    while (pos < end - start) {
        TibetanForm newState = tibetan_form(uc[pos]);
        switch (newState) {
        case TibetanSubjoinedConsonant:
        case TibetanSubjoinedVowel:
            if (state != TibetanHeadConsonant &&
                 state != TibetanSubjoinedConsonant)
                goto finish;
            state = newState;
            break;
        case TibetanVowel:
            if (state != TibetanHeadConsonant &&
                 state != TibetanSubjoinedConsonant &&
                 state != TibetanSubjoinedVowel)
                goto finish;
            break;
        case TibetanOther:
        case TibetanHeadConsonant:
            goto finish;
        }
        pos++;
    }

finish:
    *invalid = false;
    return start+pos;
}

static void tibetanAttributes(QChar::Script script, const char16_t *text, qsizetype from, qsizetype len, QCharAttributes *attributes)
{
    qsizetype end = from + len;
    qsizetype i = 0;
    Q_UNUSED(script);
    attributes += from;
    while (i < len) {
        bool invalid;
        qsizetype boundary = tibetan_nextSyllableBoundary(text, from+i, end, &invalid) - from;

        attributes[i].graphemeBoundary = true;

        if (boundary > len-1) boundary = len;
        i++;
        while (i < boundary) {
            attributes[i].graphemeBoundary = false;
            ++i;
        }
        assert(i == boundary);
    }
}

enum MymrCharClassValues {
    Mymr_CC_RESERVED = 0,
    Mymr_CC_CONSONANT = 1, /* Consonant of type 1, that has subscript form */
    Mymr_CC_CONSONANT2 = 2, /* Consonant of type 2, that has no subscript form */
    Mymr_CC_NGA = 3, /* Consonant NGA */
    Mymr_CC_YA = 4, /* Consonant YA */
    Mymr_CC_RA = 5, /* Consonant RA */
    Mymr_CC_WA = 6, /* Consonant WA */
    Mymr_CC_HA = 7, /* Consonant HA */
    Mymr_CC_IND_VOWEL = 8, /* Independent vowel */
    Mymr_CC_ZERO_WIDTH_NJ_MARK = 9, /* Zero Width non joiner character (0x200C) */
    Mymr_CC_VIRAMA = 10, /* Subscript consonant combining character */
    Mymr_CC_PRE_VOWEL = 11, /* Dependent vowel, prebase (Vowel e) */
    Mymr_CC_BELOW_VOWEL = 12, /* Dependent vowel, prebase (Vowel u, uu) */
    Mymr_CC_ABOVE_VOWEL = 13, /* Dependent vowel, prebase (Vowel i, ii, ai) */
    Mymr_CC_POST_VOWEL = 14, /* Dependent vowel, prebase (Vowel aa) */
    Mymr_CC_SIGN_ABOVE = 15,
    Mymr_CC_SIGN_BELOW = 16,
    Mymr_CC_SIGN_AFTER = 17,
    Mymr_CC_ZERO_WIDTH_J_MARK = 18, /* Zero width joiner character */
    Mymr_CC_COUNT = 19 /* This is the number of character classes */
};

enum MymrCharClassFlags {
    Mymr_CF_CLASS_MASK = 0x0000FFFF,

    Mymr_CF_CONSONANT = 0x01000000, /* flag to speed up comparing */
    Mymr_CF_MEDIAL = 0x02000000, /* flag to speed up comparing */
    Mymr_CF_IND_VOWEL = 0x04000000, /* flag to speed up comparing */
    Mymr_CF_DEP_VOWEL = 0x08000000, /* flag to speed up comparing */
    Mymr_CF_DOTTED_CIRCLE = 0x10000000, /* add a dotted circle if a character with this flag is the
                                           first in a syllable */
    Mymr_CF_VIRAMA = 0x20000000, /* flag to speed up comparing */

    /* position flags */
    Mymr_CF_POS_BEFORE = 0x00080000,
    Mymr_CF_POS_BELOW = 0x00040000,
    Mymr_CF_POS_ABOVE = 0x00020000,
    Mymr_CF_POS_AFTER = 0x00010000,
    Mymr_CF_POS_MASK = 0x000f0000,

    Mymr_CF_AFTER_KINZI = 0x00100000
};

Q_DECLARE_MIXED_ENUM_OPERATORS(int, MymrCharClassValues, MymrCharClassFlags)

/* Characters that get refrered to by name */
enum MymrChar
{
    Mymr_C_SIGN_ZWNJ     = 0x200C,
    Mymr_C_SIGN_ZWJ      = 0x200D,
    Mymr_C_DOTTED_CIRCLE = 0x25CC,
    Mymr_C_RA            = 0x101B,
    Mymr_C_YA            = 0x101A,
    Mymr_C_NGA           = 0x1004,
    Mymr_C_VOWEL_E       = 0x1031,
    Mymr_C_VIRAMA        = 0x1039
};

enum
{
    Mymr_xx = Mymr_CC_RESERVED,
    Mymr_c1 = Mymr_CC_CONSONANT | Mymr_CF_CONSONANT | Mymr_CF_POS_BELOW,
    Mymr_c2 = Mymr_CC_CONSONANT2 | Mymr_CF_CONSONANT,
    Mymr_ng = Mymr_CC_NGA | Mymr_CF_CONSONANT | Mymr_CF_POS_ABOVE,
    Mymr_ya = Mymr_CC_YA | Mymr_CF_CONSONANT | Mymr_CF_MEDIAL | Mymr_CF_POS_AFTER | Mymr_CF_AFTER_KINZI,
    Mymr_ra = Mymr_CC_RA | Mymr_CF_CONSONANT | Mymr_CF_MEDIAL | Mymr_CF_POS_BEFORE,
    Mymr_wa = Mymr_CC_WA | Mymr_CF_CONSONANT | Mymr_CF_MEDIAL | Mymr_CF_POS_BELOW,
    Mymr_ha = Mymr_CC_HA | Mymr_CF_CONSONANT | Mymr_CF_MEDIAL | Mymr_CF_POS_BELOW,
    Mymr_id = Mymr_CC_IND_VOWEL | Mymr_CF_IND_VOWEL,
    Mymr_vi = Mymr_CC_VIRAMA | Mymr_CF_VIRAMA | Mymr_CF_POS_ABOVE | Mymr_CF_DOTTED_CIRCLE,
    Mymr_dl = Mymr_CC_PRE_VOWEL | Mymr_CF_DEP_VOWEL | Mymr_CF_POS_BEFORE | Mymr_CF_DOTTED_CIRCLE | Mymr_CF_AFTER_KINZI,
    Mymr_db = Mymr_CC_BELOW_VOWEL | Mymr_CF_DEP_VOWEL | Mymr_CF_POS_BELOW | Mymr_CF_DOTTED_CIRCLE | Mymr_CF_AFTER_KINZI,
    Mymr_da = Mymr_CC_ABOVE_VOWEL | Mymr_CF_DEP_VOWEL | Mymr_CF_POS_ABOVE | Mymr_CF_DOTTED_CIRCLE | Mymr_CF_AFTER_KINZI,
    Mymr_dr = Mymr_CC_POST_VOWEL | Mymr_CF_DEP_VOWEL | Mymr_CF_POS_AFTER | Mymr_CF_DOTTED_CIRCLE | Mymr_CF_AFTER_KINZI,
    Mymr_sa = Mymr_CC_SIGN_ABOVE | Mymr_CF_DOTTED_CIRCLE | Mymr_CF_POS_ABOVE | Mymr_CF_AFTER_KINZI,
    Mymr_sb = Mymr_CC_SIGN_BELOW | Mymr_CF_DOTTED_CIRCLE | Mymr_CF_POS_BELOW | Mymr_CF_AFTER_KINZI,
    Mymr_sp = Mymr_CC_SIGN_AFTER | Mymr_CF_DOTTED_CIRCLE | Mymr_CF_AFTER_KINZI
};


typedef int MymrCharClass;


static const MymrCharClass mymrCharClasses[] =
{
    Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1, Mymr_ng, Mymr_c1, Mymr_c1, Mymr_c1,
    Mymr_c1, Mymr_c1, Mymr_c2, Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1, /* 1000 - 100F */
    Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1, Mymr_c1,
    Mymr_c1, Mymr_c1, Mymr_ya, Mymr_ra, Mymr_c1, Mymr_wa, Mymr_c1, Mymr_ha, /* 1010 - 101F */
    Mymr_c2, Mymr_c2, Mymr_xx, Mymr_id, Mymr_id, Mymr_id, Mymr_id, Mymr_id,
    Mymr_xx, Mymr_id, Mymr_id, Mymr_xx, Mymr_dr, Mymr_da, Mymr_da, Mymr_db, /* 1020 - 102F */
    Mymr_db, Mymr_dl, Mymr_da, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_sa, Mymr_sb,
    Mymr_sp, Mymr_vi, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, /* 1030 - 103F */
    Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx,
    Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, /* 1040 - 104F */
    Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx,
    Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, Mymr_xx, /* 1050 - 105F */
};

static MymrCharClass
getMyanmarCharClass (ushort ch)
{
    if (ch == Mymr_C_SIGN_ZWJ)
        return Mymr_CC_ZERO_WIDTH_J_MARK;

    if (ch == Mymr_C_SIGN_ZWNJ)
        return Mymr_CC_ZERO_WIDTH_NJ_MARK;

    if (ch < 0x1000 || ch > 0x105f)
        return Mymr_CC_RESERVED;

    return mymrCharClasses[ch - 0x1000];
}

static const signed char mymrStateTable[][Mymr_CC_COUNT] =
{
/*   xx  c1, c2  ng  ya  ra  wa  ha  id zwnj vi  dl  db  da  dr  sa  sb  sp zwj */
    { 1,  4,  4,  2,  4,  4,  4,  4, 24,  1, 27, 17, 18, 19, 20, 21,  1,  1,  4}, /*  0 - ground state */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /*  1 - exit state (or sp to the right of the syllable) */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  3, 17, 18, 19, 20, 21, -1, -1,  4}, /*  2 - NGA */
    {-1,  4,  4,  4,  4,  4,  4,  4, -1, 23, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /*  3 - Virama after NGA */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  5, 17, 18, 19, 20, 21,  1,  1, -1}, /*  4 - Base consonant */
    {-2,  6, -2, -2,  7,  8,  9, 10, -2, 23, -2, -2, -2, -2, -2, -2, -2, -2, -2}, /*  5 - First virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 25, 17, 18, 19, 20, 21, -1, -1, -1}, /*  6 - c1 after virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 17, 18, 19, 20, 21, -1, -1, -1}, /*  7 - ya after virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 17, 18, 19, 20, 21, -1, -1, -1}, /*  8 - ra after virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 17, 18, 19, 20, 21, -1, -1, -1}, /*  9 - wa after virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 17, 18, 19, 20, 21, -1, -1, -1}, /* 10 - ha after virama */
    {-1, -1, -1, -1,  7,  8,  9, 10, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* 11 - Virama after NGA+zwj */
    {-2, -2, -2, -2, -2, -2, 13, 14, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2}, /* 12 - Second virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 15, 17, 18, 19, 20, 21, -1, -1, -1}, /* 13 - wa after virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 17, 18, 19, 20, 21, -1, -1, -1}, /* 14 - ha after virama */
    {-2, -2, -2, -2, -2, -2, -2, 16, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2}, /* 15 - Third virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 17, 18, 19, 20, 21, -1, -1, -1}, /* 16 - ha after virama */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 20, 21,  1,  1, -1}, /* 17 - dl, Dependent vowel e */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 19, -1, 21,  1,  1, -1}, /* 18 - db, Dependent vowel u,uu */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1,  1, -1}, /* 19 - da, Dependent vowel i,ii,ai */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 22, -1, -1, -1, -1, -1,  1,  1, -1}, /* 20 - dr, Dependent vowel aa */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1, -1}, /* 21 - sa, Sign anusvara */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, 23, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* 22 - atha */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1,  1, -1}, /* 23 - zwnj for atha */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, -1}, /* 24 - Independent vowel */
    {-2, -2, -2, -2, 26, 26, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2, -2}, /* 25 - Virama after subscript consonant */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, 12, 17, 18, 19, 20, 21, -1,  1, -1}, /* 26 - ra/ya after subscript consonant + virama */
    {-1,  6, -1, -1,  7,  8,  9, 10, -1, 23, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /* 27 - Virama after ground state */
/* exit state -2 is for invalid order of medials and combination of invalids
   with virama where virama should treat as start of next syllable
 */
};

/*#define MYANMAR_DEBUG */
#ifdef MYANMAR_DEBUG
#define MMDEBUG qDebug
#else
#    define MMDEBUG                                                                                \
        if (0)                                                                                     \
        printf
#endif

/*
//  Given an input string of characters and a location in which to start looking
//  calculate, using the state table, which one is the last character of the syllable
//  that starts in the starting position.
*/
static qsizetype myanmar_nextSyllableBoundary(const char16_t *s, qsizetype start, qsizetype end, bool *invalid)
{
    const char16_t *uc = s + start;
    int state = 0;
    qsizetype pos = start;
    *invalid = false;

    while (pos < end) {
        MymrCharClass charClass = getMyanmarCharClass(*uc);
        state = mymrStateTable[state][charClass & Mymr_CF_CLASS_MASK];
        if (pos == start)
            *invalid = (bool)(charClass & Mymr_CF_DOTTED_CIRCLE);

        MMDEBUG("state[%lld]=%d class=%8x (uc=%4x)", qlonglong(pos - start), state, charClass, *uc);

        if (state < 0) {
            if (state < -1)
                --pos;
            break;
        }
        ++uc;
        ++pos;
    }
    return pos;
}

static void myanmarAttributes(QChar::Script script, const char16_t *text, qsizetype from, qsizetype len, QCharAttributes *attributes)
{
    qsizetype end = from + len;
    qsizetype i = 0;
    Q_UNUSED(script);
    attributes += from;
    while (i < len) {
    bool invalid;
    qsizetype boundary = myanmar_nextSyllableBoundary(text, from+i, end, &invalid) - from;

    attributes[i].graphemeBoundary = true;
    attributes[i].lineBreak = true;

    if (boundary > len-1)
            boundary = len;
    i++;
    while (i < boundary) {
        attributes[i].graphemeBoundary = false;
        ++i;
    }
    assert(i == boundary);
    }
}

/*
//  Vocabulary
//      Base ->         A consonant or an independent vowel in its full (not subscript) form. It is the
//                      center of the syllable, it can be surrounded by coeng (subscript) consonants, vowels,
//                      split vowels, signs... but there is only one base in a syllable, it has to be coded as
//                      the first character of the syllable.
//      split vowel --> vowel that has two parts placed separately (e.g. Before and after the consonant).
//                      Khmer language has five of them. Khmer split vowels either have one part before the
//                      base and one after the base or they have a part before the base and a part above the base.
//                      The first part of all Khmer split vowels is the same character, identical to
//                      the glyph of Khmer dependent vowel SRA EI
//      coeng -->  modifier used in Khmer to construct coeng (subscript) consonants
//                 Differently than indian languages, the coeng modifies the consonant that follows it,
//                 not the one preceding it  Each consonant has two forms, the base form and the subscript form
//                 the base form is the normal one (using the consonants code-point), the subscript form is
//                 displayed when the combination coeng + consonant is encountered.
//      Consonant of type 1 -> A consonant which has subscript for that only occupies space under a base consonant
//      Consonant of type 2.-> Its subscript form occupies space under and before the base (only one, RO)
//      Consonant of Type 3 -> Its subscript form occupies space under and after the base (KHO, CHHO, THHO, BA, YO, SA)
//      Consonant shifter -> Khmer has to series of consonants. The same dependent vowel has different sounds
//                           if it is attached to a consonant of the first series or a consonant of the second series
//                           Most consonants have an equivalent in the other series, but some of theme exist only in
//                           one series (for example SA). If we want to use the consonant SA with a vowel sound that
//                           can only be done with a vowel sound that corresponds to a vowel accompanying a consonant
//                           of the other series, then we need to use a consonant shifter: TRIISAP or MUSIKATOAN
//                           x17C9 y x17CA. TRIISAP changes a first series consonant to second series sound and
//                           MUSIKATOAN a second series consonant to have a first series vowel sound.
//                           Consonant shifter are both normally supercript marks, but, when they are followed by a
//                           superscript, they change shape and take the form of subscript dependent vowel SRA U.
//                           If they are in the same syllable as a coeng consonant, Unicode 3.0 says that they
//                           should be typed before the coeng. Unicode 4.0 breaks the standard and says that it should
//                           be placed after the coeng consonant.
//      Dependent vowel ->   In khmer dependent vowels can be placed above, below, before or after the base
//                           Each vowel has its own position. Only one vowel per syllable is allowed.
//      Signs            ->  Khmer has above signs and post signs. Only one above sign and/or one post sign are
//                           Allowed in a syllable.
//
//
//   order is important here! This order must be the same that is found in each horizontal
//   line in the statetable for Khmer (see khmerStateTable) .
*/
enum KhmerCharClassValues {
    CC_RESERVED             =  0,
    CC_CONSONANT            =  1, /* Consonant of type 1 or independent vowel */
    CC_CONSONANT2           =  2, /* Consonant of type 2 */
    CC_CONSONANT3           =  3, /* Consonant of type 3 */
    CC_ZERO_WIDTH_NJ_MARK   =  4, /* Zero Width non joiner character (0x200C) */
    CC_CONSONANT_SHIFTER    =  5,
    CC_ROBAT                =  6, /* Khmer special diacritic accent -treated differently in state table */
    CC_COENG                =  7, /* Subscript consonant combining character */
    CC_DEPENDENT_VOWEL      =  8,
    CC_SIGN_ABOVE           =  9,
    CC_SIGN_AFTER           = 10,
    CC_ZERO_WIDTH_J_MARK    = 11, /* Zero width joiner character */
    CC_COUNT                = 12  /* This is the number of character classes */
};


enum KhmerCharClassFlags {
    CF_CLASS_MASK    = 0x0000FFFF,

    CF_CONSONANT     = 0x01000000,  /* flag to speed up comparing */
    CF_SPLIT_VOWEL   = 0x02000000,  /* flag for a split vowel -> the first part is added in front of the syllable */
    CF_DOTTED_CIRCLE = 0x04000000,  /* add a dotted circle if a character with this flag is the first in a syllable */
    CF_COENG         = 0x08000000,  /* flag to speed up comparing */
    CF_SHIFTER       = 0x10000000,  /* flag to speed up comparing */
    CF_ABOVE_VOWEL   = 0x20000000,  /* flag to speed up comparing */

    /* position flags */
    CF_POS_BEFORE    = 0x00080000,
    CF_POS_BELOW     = 0x00040000,
    CF_POS_ABOVE     = 0x00020000,
    CF_POS_AFTER     = 0x00010000,
    CF_POS_MASK      = 0x000f0000
};

Q_DECLARE_MIXED_ENUM_OPERATORS(int, KhmerCharClassValues, KhmerCharClassFlags)

/* Characters that get referred to by name */
enum KhmerChar {
    C_SIGN_ZWNJ     = 0x200C,
    C_SIGN_ZWJ      = 0x200D,
    C_RO            = 0x179A,
    C_VOWEL_AA      = 0x17B6,
    C_SIGN_NIKAHIT  = 0x17C6,
    C_VOWEL_E       = 0x17C1,
    C_COENG         = 0x17D2
};


/*
//  simple classes, they are used in the statetable (in this file) to control the length of a syllable
//  they are also used to know where a character should be placed (location in reference to the base character)
//  and also to know if a character, when independently displayed, should be displayed with a dotted-circle to
//  indicate error in syllable construction
*/
enum {
    _xx = CC_RESERVED,
    _sa = CC_SIGN_ABOVE | CF_DOTTED_CIRCLE | CF_POS_ABOVE,
    _sp = CC_SIGN_AFTER | CF_DOTTED_CIRCLE| CF_POS_AFTER,
    _c1 = CC_CONSONANT | CF_CONSONANT,
    _c2 = CC_CONSONANT2 | CF_CONSONANT,
    _c3 = CC_CONSONANT3 | CF_CONSONANT,
    _rb = CC_ROBAT | CF_POS_ABOVE | CF_DOTTED_CIRCLE,
    _cs = CC_CONSONANT_SHIFTER | CF_DOTTED_CIRCLE | CF_SHIFTER,
    _dl = CC_DEPENDENT_VOWEL | CF_POS_BEFORE | CF_DOTTED_CIRCLE,
    _db = CC_DEPENDENT_VOWEL | CF_POS_BELOW | CF_DOTTED_CIRCLE,
    _da = CC_DEPENDENT_VOWEL | CF_POS_ABOVE | CF_DOTTED_CIRCLE | CF_ABOVE_VOWEL,
    _dr = CC_DEPENDENT_VOWEL | CF_POS_AFTER | CF_DOTTED_CIRCLE,
    _co = CC_COENG | CF_COENG | CF_DOTTED_CIRCLE,

    /* split vowel */
    _va = _da | CF_SPLIT_VOWEL,
    _vr = _dr | CF_SPLIT_VOWEL
};


/*
//   Character class: a character class value
//   ORed with character class flags.
*/
typedef unsigned long KhmerCharClass;


/*
//  Character class tables
//  _xx character does not combine into syllable, such as numbers, puntuation marks, non-Khmer signs...
//  _sa Sign placed above the base
//  _sp Sign placed after the base
//  _c1 Consonant of type 1 or independent vowel (independent vowels behave as type 1 consonants)
//  _c2 Consonant of type 2 (only RO)
//  _c3 Consonant of type 3
//  _rb Khmer sign robat u17CC. combining mark for subscript consonants
//  _cd Consonant-shifter
//  _dl Dependent vowel placed before the base (left of the base)
//  _db Dependent vowel placed below the base
//  _da Dependent vowel placed above the base
//  _dr Dependent vowel placed behind the base (right of the base)
//  _co Khmer combining mark COENG u17D2, combines with the consonant or independent vowel following
//      it to create a subscript consonant or independent vowel
//  _va Khmer split vowel in which the first part is before the base and the second one above the base
//  _vr Khmer split vowel in which the first part is before the base and the second one behind (right of) the base
*/
static const KhmerCharClass khmerCharClasses[] = {
    _c1, _c1, _c1, _c3, _c1, _c1, _c1, _c1, _c3, _c1, _c1, _c1, _c1, _c3, _c1, _c1, /* 1780 - 178F */
    _c1, _c1, _c1, _c1, _c3, _c1, _c1, _c1, _c1, _c3, _c2, _c1, _c1, _c1, _c3, _c3, /* 1790 - 179F */
    _c1, _c3, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, _c1, /* 17A0 - 17AF */
    _c1, _c1, _c1, _c1, _dr, _dr, _dr, _da, _da, _da, _da, _db, _db, _db, _va, _vr, /* 17B0 - 17BF */
    _vr, _dl, _dl, _dl, _vr, _vr, _sa, _sp, _sp, _cs, _cs, _sa, _rb, _sa, _sa, _sa, /* 17C0 - 17CF */
    _sa, _sa, _co, _sa, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _xx, _sa, _xx, _xx  /* 17D0 - 17DF */
};

/* this enum must reflect the range of khmerCharClasses */
enum KhmerCharClassesRange {
    KhmerFirstChar = 0x1780,
    KhmerLastChar  = 0x17df
};

/*
//  Below we define how a character in the input string is either in the khmerCharClasses table
//  (in which case we get its type back), a ZWJ or ZWNJ (two characters that may appear
//  within the syllable, but are not in the table) we also get their type back, or an unknown object
//  in which case we get _xx (CC_RESERVED) back
*/
static KhmerCharClass getKhmerCharClass(ushort uc)
{
    if (uc == C_SIGN_ZWJ) {
        return CC_ZERO_WIDTH_J_MARK;
    }

    if (uc == C_SIGN_ZWNJ) {
        return CC_ZERO_WIDTH_NJ_MARK;
    }

    if (uc < KhmerFirstChar || uc > KhmerLastChar) {
        return CC_RESERVED;
    }

    return khmerCharClasses[uc - KhmerFirstChar];
}


/*
//  The stateTable is used to calculate the end (the length) of a well
//  formed Khmer Syllable.
//
//  Each horizontal line is ordered exactly the same way as the values in KhmerClassTable
//  CharClassValues. This coincidence of values allows the follow up of the table.
//
//  Each line corresponds to a state, which does not necessarily need to be a type
//  of component... for example, state 2 is a base, with is always a first character
//  in the syllable, but the state could be produced a consonant of any type when
//  it is the first character that is analysed (in ground state).
//
//  Differentiating 3 types of consonants is necessary in order to
//  forbid the use of certain combinations, such as having a second
//  coeng after a coeng RO,
//  The inexistent possibility of having a type 3 after another type 3 is permitted,
//  eliminating it would very much complicate the table, and it does not create typing
//  problems, as the case above.
//
//  The table is quite complex, in order to limit the number of coeng consonants
//  to 2 (by means of the table).
//
//  There a peculiarity, as far as Unicode is concerned:
//  - The consonant-shifter is considered in two possible different
//    locations, the one considered in Unicode 3.0 and the one considered in
//    Unicode 4.0. (there is a backwards compatibility problem in this standard).
//
//
//  xx    independent character, such as a number, punctuation sign or non-khmer char
//
//  c1    Khmer consonant of type 1 or an independent vowel
//        that is, a letter in which the subscript for is only under the
//        base, not taking any space to the right or to the left
//
//  c2    Khmer consonant of type 2, the coeng form takes space under
//        and to the left of the base (only RO is of this type)
//
//  c3    Khmer consonant of type 3. Its subscript form takes space under
//        and to the right of the base.
//
//  cs    Khmer consonant shifter
//
//  rb    Khmer robat
//
//  co    coeng character (u17D2)
//
//  dv    dependent vowel (including split vowels, they are treated in the same way).
//        even if dv is not defined above, the component that is really tested for is
//        KhmerClassTable::CC_DEPENDENT_VOWEL, which is common to all dependent vowels
//
//  zwj   Zero Width joiner
//
//  zwnj  Zero width non joiner
//
//  sa    above sign
//
//  sp    post sign
//
//  there are lines with equal content but for an easier understanding
//  (and maybe change in the future) we did not join them
*/
static const signed char khmerStateTable[][CC_COUNT] =
{
    /* xx  c1  c2  c3 zwnj cs  rb  co  dv  sa  sp zwj */
    { 1,  2,  2,  2,  1,  1,  1,  6,  1,  1,  1,  2}, /*  0 - ground state */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, -1}, /*  1 - exit state (or sign to the right of the syllable) */
    {-1, -1, -1, -1,  3,  4,  5,  6, 16, 17,  1, -1}, /*  2 - Base consonant */
    {-1, -1, -1, -1, -1,  4, -1, -1, 16, -1, -1, -1}, /*  3 - First ZWNJ before a register shifter It can only be followed by a shifter or a vowel */
    {-1, -1, -1, -1, 15, -1, -1,  6, 16, 17,  1, 14}, /*  4 - First register shifter */
    {-1, -1, -1, -1, -1, -1, -1, -1, 20, -1,  1, -1}, /*  5 - Robat */
    {-1,  7,  8,  9, -1, -1, -1, -1, -1, -1, -1, -1}, /*  6 - First Coeng */
    {-1, -1, -1, -1, 12, 13, -1, 10, 16, 17,  1, 14}, /*  7 - First consonant of type 1 after coeng */
    {-1, -1, -1, -1, 12, 13, -1, -1, 16, 17,  1, 14}, /*  8 - First consonant of type 2 after coeng */
    {-1, -1, -1, -1, 12, 13, -1, 10, 16, 17,  1, 14}, /*  9 - First consonant or type 3 after ceong */
    {-1, 11, 11, 11, -1, -1, -1, -1, -1, -1, -1, -1}, /* 10 - Second Coeng (no register shifter before) */
    {-1, -1, -1, -1, 15, -1, -1, -1, 16, 17,  1, 14}, /* 11 - Second coeng consonant (or ind. vowel) no register shifter before */
    {-1, -1, -1, -1, -1, 13, -1, -1, 16, -1, -1, -1}, /* 12 - Second ZWNJ before a register shifter */
    {-1, -1, -1, -1, 15, -1, -1, -1, 16, 17,  1, 14}, /* 13 - Second register shifter */
    {-1, -1, -1, -1, -1, -1, -1, -1, 16, -1, -1, -1}, /* 14 - ZWJ before vowel */
    {-1, -1, -1, -1, -1, -1, -1, -1, 16, -1, -1, -1}, /* 15 - ZWNJ before vowel */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, 17,  1, 18}, /* 16 - dependent vowel */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, 18}, /* 17 - sign above */
    {-1, -1, -1, -1, -1, -1, -1, 19, -1, -1, -1, -1}, /* 18 - ZWJ after vowel */
    {-1,  1, -1,  1, -1, -1, -1, -1, -1, -1, -1, -1}, /* 19 - Third coeng */
    {-1, -1, -1, -1, -1, -1, -1, -1, -1, -1,  1, -1}, /* 20 - dependent vowel after a Robat */
};


/*  #define KHMER_DEBUG */
#ifdef KHMER_DEBUG
#define KHDEBUG qDebug
#else
#    define KHDEBUG                                                                                \
        if (0)                                                                                     \
        printf
#endif

/*
//  Given an input string of characters and a location in which to start looking
//  calculate, using the state table, which one is the last character of the syllable
//  that starts in the starting position.
*/
static qsizetype khmer_nextSyllableBoundary(const char16_t *s, qsizetype start, qsizetype end, bool *invalid)
{
    const char16_t *uc = s + start;
    int state = 0;
    qsizetype pos = start;
    *invalid = false;

    while (pos < end) {
        KhmerCharClass charClass = getKhmerCharClass(*uc);
        if (pos == start) {
            *invalid = (charClass > 0) && ! (charClass & CF_CONSONANT);
        }
        state = khmerStateTable[state][charClass & CF_CLASS_MASK];

        KHDEBUG("state[%lld]=%d class=%8lx (uc=%4x)", qlonglong(pos - start), state,
                charClass, *uc );

        if (state < 0) {
            break;
        }
        ++uc;
        ++pos;
    }
    return pos;
}

static void khmerAttributes(QChar::Script script, const char16_t *text, qsizetype from, qsizetype len, QCharAttributes *attributes)
{
    qsizetype end = from + len;
    qsizetype i = 0;
    Q_UNUSED(script);
    attributes += from;
    while ( i < len ) {
    bool invalid;
    qsizetype boundary = khmer_nextSyllableBoundary( text, from+i, end, &invalid ) - from;

    attributes[i].graphemeBoundary = true;

    if ( boundary > len-1 ) boundary = len;
    i++;
    while ( i < boundary ) {
        attributes[i].graphemeBoundary = false;
        ++i;
    }
    assert( i == boundary );
    }
}


static CharAttributeFunction charAttributeFunction(QChar::Script script)
{
    switch (script) {
    case QChar::Script_Unknown:
    case QChar::Script_Inherited:
    case QChar::Script_Common:
    case QChar::Script_Latin:
    case QChar::Script_Greek:
    case QChar::Script_Cyrillic:
    case QChar::Script_Armenian:
    case QChar::Script_Hebrew:
    case QChar::Script_Arabic:
    case QChar::Script_Syriac:
    case QChar::Script_Thaana:
        return nullptr;
    case QChar::Script_Devanagari:
    case QChar::Script_Bengali:
    case QChar::Script_Gurmukhi:
    case QChar::Script_Gujarati:
    case QChar::Script_Oriya:
    case QChar::Script_Tamil:
    case QChar::Script_Telugu:
    case QChar::Script_Kannada:
    case QChar::Script_Malayalam:
    case QChar::Script_Sinhala:
        return &indicAttributes;
    case QChar::Script_Thai:
        return &thaiAttributes;
    case QChar::Script_Lao:
        return nullptr;
    case QChar::Script_Tibetan:
        return &tibetanAttributes;
    case QChar::Script_Myanmar:
        return &myanmarAttributes;
    case QChar::Script_Georgian:
    case QChar::Script_Hangul:
    case QChar::Script_Ethiopic:
    case QChar::Script_Cherokee:
    case QChar::Script_CanadianAboriginal:
    case QChar::Script_Ogham:
    case QChar::Script_Runic:
        return nullptr;
    case QChar::Script_Khmer:
        return &khmerAttributes;
    case QChar::Script_Mongolian:
    case QChar::Script_Hiragana:
    case QChar::Script_Katakana:
    case QChar::Script_Bopomofo:
    case QChar::Script_Han:
    case QChar::Script_Yi:
    case QChar::Script_OldItalic:
    case QChar::Script_Gothic:
    case QChar::Script_Deseret:
    case QChar::Script_Tagalog:
    case QChar::Script_Hanunoo:
    case QChar::Script_Buhid:
    case QChar::Script_Tagbanwa:
    case QChar::Script_Coptic:
    case QChar::Script_Limbu:
    case QChar::Script_TaiLe:
    case QChar::Script_LinearB:
    case QChar::Script_Ugaritic:
    case QChar::Script_Shavian:
    case QChar::Script_Osmanya:
    case QChar::Script_Cypriot:
    case QChar::Script_Braille:
    case QChar::Script_Buginese:
    case QChar::Script_NewTaiLue:
    case QChar::Script_Glagolitic:
    case QChar::Script_Tifinagh:
    case QChar::Script_SylotiNagri:
    case QChar::Script_OldPersian:
    case QChar::Script_Kharoshthi:
    case QChar::Script_Balinese:
    case QChar::Script_Cuneiform:
    case QChar::Script_Phoenician:
    case QChar::Script_PhagsPa:
    case QChar::Script_Nko:
    case QChar::Script_Sundanese:
    case QChar::Script_Lepcha:
    case QChar::Script_OlChiki:
    case QChar::Script_Vai:
    case QChar::Script_Saurashtra:
    case QChar::Script_KayahLi:
    case QChar::Script_Rejang:
    case QChar::Script_Lycian:
    case QChar::Script_Carian:
    case QChar::Script_Lydian:
    case QChar::Script_Cham:
    case QChar::Script_TaiTham:
    case QChar::Script_TaiViet:
    case QChar::Script_Avestan:
    case QChar::Script_EgyptianHieroglyphs:
    case QChar::Script_Samaritan:
    case QChar::Script_Lisu:
    case QChar::Script_Bamum:
    case QChar::Script_Javanese:
    case QChar::Script_MeeteiMayek:
    case QChar::Script_ImperialAramaic:
    case QChar::Script_OldSouthArabian:
    case QChar::Script_InscriptionalParthian:
    case QChar::Script_InscriptionalPahlavi:
    case QChar::Script_OldTurkic:
    case QChar::Script_Kaithi:
    case QChar::Script_Batak:
    case QChar::Script_Brahmi:
    case QChar::Script_Mandaic:
    case QChar::Script_Chakma:
    case QChar::Script_MeroiticCursive:
    case QChar::Script_MeroiticHieroglyphs:
    case QChar::Script_Miao:
    case QChar::Script_Sharada:
    case QChar::Script_SoraSompeng:
    case QChar::Script_Takri:
    case QChar::Script_CaucasianAlbanian:
    case QChar::Script_BassaVah:
    case QChar::Script_Duployan:
    case QChar::Script_Elbasan:
    case QChar::Script_Grantha:
    case QChar::Script_PahawhHmong:
    case QChar::Script_Khojki:
    case QChar::Script_LinearA:
    case QChar::Script_Mahajani:
    case QChar::Script_Manichaean:
    case QChar::Script_MendeKikakui:
    case QChar::Script_Modi:
    case QChar::Script_Mro:
    case QChar::Script_OldNorthArabian:
    case QChar::Script_Nabataean:
    case QChar::Script_Palmyrene:
    case QChar::Script_PauCinHau:
    case QChar::Script_OldPermic:
    case QChar::Script_PsalterPahlavi:
    case QChar::Script_Siddham:
    case QChar::Script_Khudawadi:
    case QChar::Script_Tirhuta:
    case QChar::Script_WarangCiti:
    case QChar::Script_Ahom:
    case QChar::Script_AnatolianHieroglyphs:
    case QChar::Script_Hatran:
    case QChar::Script_Multani:
    case QChar::Script_OldHungarian:
    case QChar::Script_SignWriting:
    case QChar::Script_Adlam:
    case QChar::Script_Bhaiksuki:
    case QChar::Script_Marchen:
    case QChar::Script_Newa:
    case QChar::Script_Osage:
    case QChar::Script_Tangut:
    case QChar::Script_MasaramGondi:
    case QChar::Script_Nushu:
    case QChar::Script_Soyombo:
    case QChar::Script_ZanabazarSquare:
    case QChar::Script_Dogra:
    case QChar::Script_GunjalaGondi:
    case QChar::Script_HanifiRohingya:
    case QChar::Script_Makasar:
    case QChar::Script_Medefaidrin:
    case QChar::Script_OldSogdian:
    case QChar::Script_Sogdian:
    case QChar::Script_Elymaic:
    case QChar::Script_Nandinagari:
    case QChar::Script_NyiakengPuachueHmong:
    case QChar::Script_Wancho:
    case QChar::Script_Chorasmian:
    case QChar::Script_DivesAkuru:
    case QChar::Script_KhitanSmallScript:
    case QChar::Script_Yezidi:
    case QChar::Script_CyproMinoan:
    case QChar::Script_OldUyghur:
    case QChar::Script_Tangsa:
    case QChar::Script_Toto:
    case QChar::Script_Vithkuqi:
    case QChar::Script_Kawi:
    case QChar::Script_NagMundari:
    case QChar::Script_Garay:
    case QChar::Script_GurungKhema:
    case QChar::Script_KiratRai:
    case QChar::Script_OlOnal:
    case QChar::Script_Sunuwar:
    case QChar::Script_Todhri:
    case QChar::Script_TuluTigalari:
        return nullptr;
    case QChar::ScriptCount:
        // Don't Q_UNREACHABLE here; this might be a newer value in later Qt versions
        // (incl. patch releases)
        ;
    }
    return nullptr;
};

static void getCharAttributes(const char16_t *string, qsizetype stringLength,
                                  const QUnicodeTools::ScriptItem *items, qsizetype numItems,
                                  QCharAttributes *attributes)
{
    if (stringLength == 0)
        return;
    for (qsizetype i = 0; i < numItems; ++i) {
        QChar::Script script = items[i].script;
        CharAttributeFunction attributeFunction = charAttributeFunction(script);
        if (!attributeFunction)
            continue;
        qsizetype end = i < numItems - 1 ? items[i + 1].position : stringLength;
        attributeFunction(script, string, items[i].position, end - items[i].position, attributes);
    }
}

}

Q_CORE_EXPORT void initCharAttributes(QStringView string,
                                      const ScriptItem *items, qsizetype numItems,
                                      QCharAttributes *attributes, CharAttributeOptions options)
{
    if (string.size() <= 0)
        return;

    if (!(options & DontClearAttributes))
        ::memset(attributes, 0, (string.size() + 1) * sizeof(QCharAttributes));

    if (options & GraphemeBreaks)
        getGraphemeBreaks(string.utf16(), string.size(), attributes);
    if (options & WordBreaks)
        getWordBreaks(string.utf16(), string.size(), attributes);
    if (options & SentenceBreaks)
        getSentenceBreaks(string.utf16(), string.size(), attributes);
    if (options & LineBreaks)
        getLineBreaks(string.utf16(), string.size(), attributes, options);
    if (options & WhiteSpaces)
        getWhiteSpaces(string.utf16(), string.size(), attributes);

    if (!qt_initcharattributes_default_algorithm_only) {
        if (!items || numItems <= 0)
            return;

        Tailored::getCharAttributes(string.utf16(), string.size(), items, numItems, attributes);
    }
}


// ----------------------------------------------------------------------------
//
// The Unicode script property. See http://www.unicode.org/reports/tr24/tr24-24.html
//
// ----------------------------------------------------------------------------

Q_CORE_EXPORT void initScripts(QStringView string, ScriptItemArray *scripts)
{
    qsizetype sor = 0;
    qsizetype eor = 0;
    QChar::Script script = QChar::Script_Common;

    for (qsizetype i = 0; i < string.size(); ++i, eor = i) {
        char32_t ucs4 = string[i].unicode();
        if (QChar::isHighSurrogate(ucs4) && i + 1 < string.size()) {
            ushort low = string[i + 1].unicode();
            if (QChar::isLowSurrogate(low)) {
                ucs4 = QChar::surrogateToUcs4(ucs4, low);
                ++i;
            }
        }

        const QUnicodeTables::Properties *prop = QUnicodeTables::properties(ucs4);

        QChar::Script nscript = QChar::Script(prop->script);

        if (Q_LIKELY(nscript == script || nscript <= QChar::Script_Common))
            continue;

        // inherit preceding Common-s
        if (Q_UNLIKELY(script <= QChar::Script_Common)) {
            // also covers a case where the base character of Common script followed
            // by one or more combining marks of non-Inherited, non-Common script
            script = nscript;
            continue;
        }

        // Never break between a combining mark (gc= Mc, Mn or Me) and its base character.
        // Thus, a combining mark - whatever its script property value is - should inherit
        // the script property value of its base character.
        static const int test = (FLAG(QChar::Mark_NonSpacing) | FLAG(QChar::Mark_SpacingCombining) | FLAG(QChar::Mark_Enclosing));
        if (Q_UNLIKELY(FLAG(prop->category) & test))
            continue;

        Q_ASSERT(script > QChar::Script_Common);
        Q_ASSERT(sor < eor);
        scripts->append(ScriptItem{sor, script});
        sor = eor;

        script = nscript;
    }

    Q_ASSERT(script >= QChar::Script_Common);
    Q_ASSERT(eor == string.size());
    scripts->append(ScriptItem{sor, script});
}

} // namespace QUnicodeTools

QT_END_NAMESPACE
