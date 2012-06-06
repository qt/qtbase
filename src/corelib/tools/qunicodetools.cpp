/****************************************************************************
**
** Copyright (C) 2012 Nokia Corporation and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qunicodetools_p.h"

#include "qunicodetables_p.h"

QT_BEGIN_NAMESPACE

Q_AUTOTEST_EXPORT int qt_initcharattributes_default_algorithm_only = 0;

namespace QUnicodeTools {

// -----------------------------------------------------------------------------------------------------
//
// The line breaking algorithm. See http://www.unicode.org/reports/tr14/tr14-19.html
//
// -----------------------------------------------------------------------------------------------------
//
// The text boundaries determination algorithm. See http://www.unicode.org/reports/tr29/tr29-11.html
//
// -----------------------------------------------------------------------------------------------------

/* The Unicode algorithm does in our opinion allow line breaks at some
   places they shouldn't be allowed. The following changes were thus
   made in comparison to the Unicode reference:

   EX->AL from DB to IB
   SY->AL from DB to IB
   SY->PO from DB to IB
   SY->PR from DB to IB
   SY->OP from DB to IB
   AL->PR from DB to IB
   AL->PO from DB to IB
   PR->PR from DB to IB
   PO->PO from DB to IB
   PR->PO from DB to IB
   PO->PR from DB to IB
   HY->PO from DB to IB
   HY->PR from DB to IB
   HY->OP from DB to IB
   NU->EX from PB to IB
   EX->PO from DB to IB
*/

// The following line break classes are not treated by the table:
//  AI, BK, CB, CR, LF, NL, SA, SG, SP, XX

enum LineBreakRule {
    ProhibitedBreak,            // PB in table
    DirectBreak,                // DB in table
    IndirectBreak,              // IB in table
    CombiningIndirectBreak,     // CI in table
    CombiningProhibitedBreak    // CP in table
};
#define DB DirectBreak
#define IB IndirectBreak
#define CI CombiningIndirectBreak
#define CP CombiningProhibitedBreak
#define PB ProhibitedBreak
static const uchar lineBreakTable[QUnicodeTables::LineBreak_JT + 1][QUnicodeTables::LineBreak_JT + 1] = {
/*         OP  CL  QU  GL  NS  EX  SY  IS  PR  PO  NU  AL  ID  IN  HY  BA  BB  B2  ZW  CM  WJ  H2  H3  JL  JV  JT */
/* OP */ { PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, PB, CP, PB, PB, PB, PB, PB, PB },
/* CL */ { DB, PB, IB, IB, PB, PB, PB, PB, IB, IB, IB, IB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* QU */ { PB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB },
/* GL */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB },
/* NS */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* EX */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, IB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* SY */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* IS */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, DB, IB, IB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* PR */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, DB, IB, IB, DB, DB, PB, CI, PB, IB, IB, IB, IB, IB },
/* PO */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* NU */ { IB, PB, IB, IB, IB, IB, PB, PB, IB, IB, IB, IB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* AL */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* ID */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* IN */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* HY */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, DB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* BA */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* BB */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB },
/* B2 */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, DB, DB, DB, DB, DB, IB, IB, DB, PB, PB, CI, PB, DB, DB, DB, DB, DB },
/* ZW */ { DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, DB, PB, DB, DB, DB, DB, DB, DB, DB },
/* CM */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, DB, IB, IB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, DB },
/* WJ */ { IB, PB, IB, IB, IB, PB, PB, PB, IB, IB, IB, IB, IB, IB, IB, IB, IB, IB, PB, CI, PB, IB, IB, IB, IB, IB },
/* H2 */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, IB, IB },
/* H3 */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, IB },
/* JL */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, IB, IB, IB, IB, DB },
/* JV */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, IB, IB },
/* JT */ { DB, PB, IB, IB, IB, PB, PB, PB, DB, IB, DB, DB, DB, IB, IB, IB, DB, DB, PB, CI, PB, DB, DB, DB, DB, IB }
};
#undef DB
#undef IB
#undef CI
#undef CP
#undef PB

static const uchar graphemeBreakTable[QUnicodeTables::GraphemeBreakLVT + 1][QUnicodeTables::GraphemeBreakLVT + 1] = {
//    Other, CR,    LF,  Control, Extend, L,     V,     T,     LV,    LVT
    { true , true , true , true , true , true , true , true , true , true  }, // Other,
    { true , true , true , true , true , true , true , true , true , true  }, // CR,
    { true , false, true , true , true , true , true , true , true , true  }, // LF,
    { true , true , true , true , true , true , true , true , true , true  }, // Control,
    { false, true , true , true , false, false, false, false, false, false }, // Extend,
    { true , true , true , true , true , false, true , true , true , true  }, // L,
    { true , true , true , true , true , false, false, true , false, true  }, // V,
    { true , true , true , true , true , true , false, false, false, false }, // T,
    { true , true , true , true , true , false, true , true , true , true  }, // LV,
    { true , true , true , true , true , false, true , true , true , true  }, // LVT
};

static void calcGraphemeAndLineBreaks(const ushort *string, quint32 len, HB_CharAttributes *attributes)
{
    // ##### can this fail if the first char is a surrogate?
    const QUnicodeTables::Properties *prop = QUnicodeTables::properties(string[0]);
    QUnicodeTables::GraphemeBreak grapheme = (QUnicodeTables::GraphemeBreak) prop->graphemeBreak;
    QUnicodeTables::LineBreakClass cls = (QUnicodeTables::LineBreakClass) prop->line_break_class;
    // handle case where input starts with an LF
    if (cls == QUnicodeTables::LineBreak_LF)
        cls = QUnicodeTables::LineBreak_BK;

    attributes[0].charStop = true;

    int lcls = cls;
    for (quint32 i = 1; i < len; ++i) {
        attributes[i].charStop = true;

        uint ucs4 = string[i];
        prop = QUnicodeTables::properties(ucs4);
        QUnicodeTables::GraphemeBreak ngrapheme = (QUnicodeTables::GraphemeBreak) prop->graphemeBreak;
        QUnicodeTables::LineBreakClass ncls = (QUnicodeTables::LineBreakClass) prop->line_break_class;
        attributes[i].charStop = graphemeBreakTable[ngrapheme][grapheme];
        // handle surrogates
        if (ncls == QUnicodeTables::LineBreak_SG) {
            if (QChar::isHighSurrogate(string[i]) && i < len - 1 && QChar::isLowSurrogate(string[i+1])) {
                continue;
            } else if (QChar::isLowSurrogate(string[i]) && QChar::isHighSurrogate(string[i-1])) {
                ucs4 = QChar::surrogateToUcs4(string[i-1], string[i]);
                prop = QUnicodeTables::properties(ucs4);
                ngrapheme = (QUnicodeTables::GraphemeBreak) prop->graphemeBreak;
                ncls = (QUnicodeTables::LineBreakClass) prop->line_break_class;
                attributes[i].charStop = false;
            } else {
                ncls = QUnicodeTables::LineBreak_AL;
            }
        }

        HB_LineBreakType lineBreakType = HB_NoBreak;

        if (cls >= QUnicodeTables::LineBreak_CR) {
            if (cls > QUnicodeTables::LineBreak_CR || ncls != QUnicodeTables::LineBreak_LF)
                lineBreakType = HB_ForcedBreak;
            goto next;
        }

        if (ncls == QUnicodeTables::LineBreak_SP)
            goto next_no_cls_update;
        if (ncls >= QUnicodeTables::LineBreak_CR)
            goto next;

        {
            int tcls = ncls;
            // for south east asian chars that require a complex (dictionary analysis), the unicode
            // standard recommends to treat them as AL. thai_attributes and other attribute methods that
            // do dictionary analysis can override
            if (tcls >= QUnicodeTables::LineBreak_SA)
                tcls = QUnicodeTables::LineBreak_AL;
            if (cls >= QUnicodeTables::LineBreak_SA)
                cls = QUnicodeTables::LineBreak_AL;

            int brk = lineBreakTable[cls][tcls];
            switch (brk) {
            case DirectBreak:
                lineBreakType = HB_Break;
                if (string[i-1] == 0xad) // soft hyphen
                    lineBreakType = HB_SoftHyphen;
                break;
            case IndirectBreak:
                lineBreakType = (lcls == QUnicodeTables::LineBreak_SP) ? HB_Break : HB_NoBreak;
                break;
            case CombiningIndirectBreak:
                lineBreakType = HB_NoBreak;
                if (lcls == QUnicodeTables::LineBreak_SP){
                    if (i > 1)
                        attributes[i-2].lineBreakType = HB_Break;
                } else {
                    goto next_no_cls_update;
                }
                break;
            case CombiningProhibitedBreak:
                lineBreakType = HB_NoBreak;
                if (lcls != QUnicodeTables::LineBreak_SP)
                    goto next_no_cls_update;
            case ProhibitedBreak:
            default:
                break;
            }
        }
    next:
        cls = ncls;
    next_no_cls_update:
        lcls = ncls;
        grapheme = ngrapheme;
        attributes[i-1].lineBreakType = lineBreakType;
    }

    for (quint32 i = len - 1; i > 0; --i)
        attributes[i].lineBreakType = attributes[i - 1].lineBreakType;
    attributes[0].lineBreakType = HB_NoBreak; // LB2
}


enum WordBreakRule { NoBreak = 0, Break = 1, Middle = 2 };

static const uchar wordBreakTable[QUnicodeTables::WordBreakExtendNumLet + 1][QUnicodeTables::WordBreakExtendNumLet + 1] = {
//    Other    Format   Katakana ALetter  MidLetter MidNum  Numeric  ExtendNumLet
    { Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // Other
    { Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // Format
    { Break  , Break  , NoBreak, Break  , Break  , Break  , Break  , NoBreak }, // Katakana
    { Break  , Break  , Break  , NoBreak, Middle , Break  , NoBreak, NoBreak }, // ALetter
    { Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // MidLetter
    { Break  , Break  , Break  , Break  , Break  , Break  , Break  , Break   }, // MidNum
    { Break  , Break  , Break  , NoBreak, Break  , Middle , NoBreak, NoBreak }, // Numeric
    { Break  , Break  , NoBreak, NoBreak, Break  , Break  , NoBreak, NoBreak }, // ExtendNumLet
};

static void calcWordBreaks(const ushort *string, quint32 len, HB_CharAttributes *attributes)
{
    quint32 brk = QUnicodeTables::wordBreakClass(string[0]);

    attributes[0].wordBoundary = true;

    for (quint32 i = 1; i < len; ++i) {
        if (!attributes[i].charStop) {
            attributes[i].wordBoundary = false;
            continue;
        }

        quint32 nbrk = QUnicodeTables::wordBreakClass(string[i]);
        if (nbrk == QUnicodeTables::WordBreakFormat) {
            attributes[i].wordBoundary = (QUnicodeTables::sentenceBreakClass(string[i-1]) == QUnicodeTables::SentenceBreakSep);
            continue;
        }

        WordBreakRule rule = (WordBreakRule)wordBreakTable[brk][nbrk];
        if (rule == Middle) {
            rule = Break;
            quint32 lookahead = i + 1;
            while (lookahead < len) {
                quint32 testbrk = QUnicodeTables::wordBreakClass(string[lookahead]);
                if (testbrk == QUnicodeTables::WordBreakFormat
                    && QUnicodeTables::sentenceBreakClass(string[lookahead]) != QUnicodeTables::SentenceBreakSep) {
                    ++lookahead;
                    continue;
                }
                if (testbrk == brk) {
                    rule = NoBreak;
                    while (i < lookahead)
                        attributes[i++].wordBoundary = false;
                    nbrk = testbrk;
                }
                break;
            }
        }
        attributes[i].wordBoundary = (rule == Break);
        brk = nbrk;
    }
}


enum SentenceBreakState {
    SB_Initial,
    SB_Upper,
    SB_UpATerm,
    SB_ATerm,
    SB_ATermC,
    SB_ACS,
    SB_STerm,
    SB_STermC,
    SB_SCS,
    SB_BAfter,
    SB_Break,
    SB_Lookup
};

static const uchar sentenceBreakTable[SB_Lookup + 1][QUnicodeTables::SentenceBreakClose + 1] = {
//      Other       Sep         Format      Sp          Lower       Upper       OLetter     Numeric     ATerm       STerm       Close
    { SB_Initial, SB_BAfter , SB_Initial, SB_Initial, SB_Initial, SB_Upper  , SB_Initial, SB_Initial, SB_ATerm  , SB_STerm  , SB_Initial }, // SB_Initial,
    { SB_Initial, SB_BAfter , SB_Upper  , SB_Initial, SB_Initial, SB_Upper  , SB_Initial, SB_Initial, SB_UpATerm, SB_STerm  , SB_Initial }, // SB_Upper

    { SB_Lookup , SB_BAfter , SB_UpATerm, SB_ACS    , SB_Initial, SB_Upper  , SB_Break  , SB_Initial, SB_ATerm  , SB_STerm  , SB_ATermC  }, // SB_UpATerm
    { SB_Lookup , SB_BAfter , SB_ATerm  , SB_ACS    , SB_Initial, SB_Break  , SB_Break  , SB_Initial, SB_ATerm  , SB_STerm  , SB_ATermC  }, // SB_ATerm
    { SB_Lookup , SB_BAfter , SB_ATermC , SB_ACS    , SB_Initial, SB_Break  , SB_Break  , SB_Lookup , SB_ATerm  , SB_STerm  , SB_ATermC  }, // SB_ATermC,
    { SB_Lookup , SB_BAfter , SB_ACS    , SB_ACS    , SB_Initial, SB_Break  , SB_Break  , SB_Lookup , SB_ATerm  , SB_STerm  , SB_Lookup  }, // SB_ACS,

    { SB_Break  , SB_BAfter , SB_STerm  , SB_SCS    , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_ATerm  , SB_STerm  , SB_STermC  }, // SB_STerm,
    { SB_Break  , SB_BAfter , SB_STermC , SB_SCS    , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_ATerm  , SB_STerm  , SB_STermC  }, // SB_STermC,
    { SB_Break  , SB_BAfter , SB_SCS    , SB_SCS    , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_ATerm  , SB_STerm  , SB_Break   }, // SB_SCS,
    { SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_Break  , SB_Break   }, // SB_BAfter,
};

static void calcSentenceBreaks(const ushort *string, quint32 len, HB_CharAttributes *attributes)
{
    quint32 brk = sentenceBreakTable[SB_Initial][QUnicodeTables::sentenceBreakClass(string[0])];
    attributes[0].sentenceBoundary = true;
    for (quint32 i = 1; i < len; ++i) {
        if (!attributes[i].charStop) {
            attributes[i].sentenceBoundary = false;
            continue;
        }
        brk = sentenceBreakTable[brk][QUnicodeTables::sentenceBreakClass(string[i])];
        if (brk == SB_Lookup) {
            brk = SB_Break;
            quint32 lookahead = i + 1;
            while (lookahead < len) {
                quint32 sbrk = QUnicodeTables::sentenceBreakClass(string[lookahead]);
                if (sbrk != QUnicodeTables::SentenceBreakOther
                    && sbrk != QUnicodeTables::SentenceBreakNumeric
                    && sbrk != QUnicodeTables::SentenceBreakClose) {
                    break;
                } else if (sbrk == QUnicodeTables::SentenceBreakLower) {
                    brk = SB_Initial;
                    break;
                }
                ++lookahead;
            }
            if (brk == SB_Initial) {
                while (i < lookahead)
                    attributes[i++].sentenceBoundary = false;
            }
        }
        if (brk == SB_Break) {
            attributes[i].sentenceBoundary = true;
            brk = sentenceBreakTable[SB_Initial][QUnicodeTables::sentenceBreakClass(string[i])];
        } else {
            attributes[i].sentenceBoundary = false;
        }
    }
}


static void getWhiteSpaces(const ushort *string, quint32 len, HB_CharAttributes *attributes)
{
    for (quint32 i = 0; i != len; ++i) {
        uint ucs4 = string[i];
        if (QChar::isHighSurrogate(ucs4) && i + 1 != len) {
            ushort low = string[i + 1];
            if (QChar::isLowSurrogate(low)) {
                ucs4 = QChar::surrogateToUcs4(ucs4, low);
                ++i;
            }
        }

        if (QChar::isSpace(ucs4))
            attributes[i].whiteSpace = true;
    }
}


Q_CORE_EXPORT void initCharAttributes(const ushort *string, int length,
                                      const HB_ScriptItem *items, int numItems,
                                      HB_CharAttributes *attributes, CharAttributeOptions options)
{
    if (length <= 0)
        return;

    if (!(options & DontClearAttributes)) {
        ::memset(attributes, 0, length * sizeof(HB_CharAttributes));
        if (options & (WordBreaks | SentenceBreaks))
            options |= GraphemeBreaks;
    }

    if (options & (GraphemeBreaks | LineBreaks))
        calcGraphemeAndLineBreaks(string, length, attributes);
    if (options & WordBreaks)
        calcWordBreaks(string, length, attributes);
    if (options & SentenceBreaks)
        calcSentenceBreaks(string, length, attributes);
    if (options & WhiteSpaces)
        getWhiteSpaces(string, length, attributes);

    if (!items || numItems <= 0)
        return;
    if (!qt_initcharattributes_default_algorithm_only)
        HB_GetTailoredCharAttributes(string, length, items, numItems, attributes);
}

} // namespace QUnicodeTools

QT_END_NAMESPACE
