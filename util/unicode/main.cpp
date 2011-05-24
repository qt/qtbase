/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the utils of the Qt Toolkit.
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
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qlist.h>
#include <qhash.h>
#include <qfile.h>
#include <qbytearray.h>
#include <qstring.h>
#include <qchar.h>
#include <qvector.h>
#include <qdebug.h>
#if 0
#include <private/qunicodetables_p.h>
#endif

#define DATA_VERSION_S "5.0"
#define DATA_VERSION_STR "QChar::Unicode_5_0"

#define LAST_CODEPOINT 0x10ffff
#define LAST_CODEPOINT_STR "0x10ffff"


static QHash<QByteArray, QChar::UnicodeVersion> age_map;

static void initAgeMap()
{
    struct AgeMap {
        const QChar::UnicodeVersion version;
        const char *age;
    } ageMap[] = {
        { QChar::Unicode_1_1,   "1.1" },
        { QChar::Unicode_2_0,   "2.0" },
        { QChar::Unicode_2_1_2, "2.1" },
        { QChar::Unicode_3_0,   "3.0" },
        { QChar::Unicode_3_1,   "3.1" },
        { QChar::Unicode_3_2,   "3.2" },
        { QChar::Unicode_4_0,   "4.0" },
        { QChar::Unicode_4_1,   "4.1" },
        { QChar::Unicode_5_0,   "5.0" },
        { QChar::Unicode_Unassigned, 0 }
    };
    AgeMap *d = ageMap;
    while (d->age) {
        age_map.insert(d->age, d->version);
        ++d;
    }
}


enum Joining {
    Joining_None,
    Joining_Left,
    Joining_Causing,
    Joining_Dual,
    Joining_Right,
    Joining_Transparent

    , Joining_Unassigned
};

static QHash<QByteArray, Joining> joining_map;

static void initJoiningMap()
{
    struct JoiningList {
        Joining joining;
        const char *name;
    } joinings[] = {
        { Joining_None,        "U" },
        { Joining_Left,        "L" },
        { Joining_Causing,     "C" },
        { Joining_Dual,        "D" },
        { Joining_Right,       "R" },
        { Joining_Transparent, "T" },
        { Joining_Unassigned, 0 }
    };
    JoiningList *d = joinings;
    while (d->name) {
        joining_map.insert(d->name, d->joining);
        ++d;
    }
}


static const char *grapheme_break_string =
    "    enum GraphemeBreak {\n"
    "        GraphemeBreakOther,\n"
    "        GraphemeBreakCR,\n"
    "        GraphemeBreakLF,\n"
    "        GraphemeBreakControl,\n"
    "        GraphemeBreakExtend,\n"
    "        GraphemeBreakL,\n"
    "        GraphemeBreakV,\n"
    "        GraphemeBreakT,\n"
    "        GraphemeBreakLV,\n"
    "        GraphemeBreakLVT\n"
    "    };\n\n";

enum GraphemeBreak {
    GraphemeBreakOther,
    GraphemeBreakCR,
    GraphemeBreakLF,
    GraphemeBreakControl,
    GraphemeBreakExtend,
    GraphemeBreakL,
    GraphemeBreakV,
    GraphemeBreakT,
    GraphemeBreakLV,
    GraphemeBreakLVT

    , GraphemeBreak_Unassigned
};

static QHash<QByteArray, GraphemeBreak> grapheme_break_map;

static void initGraphemeBreak()
{
    struct GraphemeBreakList {
        GraphemeBreak brk;
        const char *name;
    } breaks[] = {
        { GraphemeBreakOther, "Other" },
        { GraphemeBreakCR, "CR" },
        { GraphemeBreakLF, "LF" },
        { GraphemeBreakControl, "Control" },
        { GraphemeBreakExtend, "Extend" },
        { GraphemeBreakL, "L" },
        { GraphemeBreakV, "V" },
        { GraphemeBreakT, "T" },
        { GraphemeBreakLV, "LV" },
        { GraphemeBreakLVT, "LVT" },
        { GraphemeBreak_Unassigned, 0 }
    };
    GraphemeBreakList *d = breaks;
    while (d->name) {
        grapheme_break_map.insert(d->name, d->brk);
        ++d;
    }
}


static const char *word_break_string =
    "    enum WordBreak {\n"
    "        WordBreakOther,\n"
    "        WordBreakFormat,\n"
    "        WordBreakKatakana,\n"
    "        WordBreakALetter,\n"
    "        WordBreakMidLetter,\n"
    "        WordBreakMidNum,\n"
    "        WordBreakNumeric,\n"
    "        WordBreakExtendNumLet\n"
    "    };\n\n";

enum WordBreak {
    WordBreakOther,
    WordBreakFormat,
    WordBreakKatakana,
    WordBreakALetter,
    WordBreakMidLetter,
    WordBreakMidNum,
    WordBreakNumeric,
    WordBreakExtendNumLet

    , WordBreak_Unassigned
};

static QHash<QByteArray, WordBreak> word_break_map;

static void initWordBreak()
{
    struct WordBreakList {
        WordBreak brk;
        const char *name;
    } breaks[] = {
        { WordBreakFormat, "Format" },
        { WordBreakFormat, "Extend" }, // these are copied in from GraphemeBreakProperty.txt
        { WordBreakKatakana, "Katakana" },
        { WordBreakALetter, "ALetter" },
        { WordBreakMidLetter, "MidLetter" },
        { WordBreakMidNum, "MidNum" },
        { WordBreakNumeric, "Numeric" },
        { WordBreakExtendNumLet, "ExtendNumLet" },
        { WordBreak_Unassigned, 0 }
    };
    WordBreakList *d = breaks;
    while (d->name) {
        word_break_map.insert(d->name, d->brk);
        ++d;
    }
}


static const char *sentence_break_string =
    "    enum SentenceBreak {\n"
    "        SentenceBreakOther,\n"
    "        SentenceBreakSep,\n"
    "        SentenceBreakFormat,\n"
    "        SentenceBreakSp,\n"
    "        SentenceBreakLower,\n"
    "        SentenceBreakUpper,\n"
    "        SentenceBreakOLetter,\n"
    "        SentenceBreakNumeric,\n"
    "        SentenceBreakATerm,\n"
    "        SentenceBreakSTerm,\n"
    "        SentenceBreakClose\n"
    "    };\n\n";

enum SentenceBreak {
    SentenceBreakOther,
    SentenceBreakSep,
    SentenceBreakFormat,
    SentenceBreakSp,
    SentenceBreakLower,
    SentenceBreakUpper,
    SentenceBreakOLetter,
    SentenceBreakNumeric,
    SentenceBreakATerm,
    SentenceBreakSTerm,
    SentenceBreakClose

    , SentenceBreak_Unassigned
};

static QHash<QByteArray, SentenceBreak> sentence_break_map;

static void initSentenceBreak()
{
    struct SentenceBreakList {
        SentenceBreak brk;
        const char *name;
    } breaks[] = {
        { SentenceBreakOther, "Other" },
        { SentenceBreakSep, "Sep" },
        { SentenceBreakFormat, "Format" },
        { SentenceBreakSp, "Sp" },
        { SentenceBreakLower, "Lower" },
        { SentenceBreakUpper, "Upper" },
        { SentenceBreakOLetter, "OLetter" },
        { SentenceBreakNumeric, "Numeric" },
        { SentenceBreakATerm, "ATerm" },
        { SentenceBreakSTerm, "STerm" },
        { SentenceBreakClose, "Close" },
        { SentenceBreak_Unassigned, 0 }
    };
    SentenceBreakList *d = breaks;
    while (d->name) {
        sentence_break_map.insert(d->name, d->brk);
        ++d;
    }
}


static const char *lineBreakClass =
    "    // see http://www.unicode.org/reports/tr14/tr14-19.html\n"
    "    // we don't use the XX, AI and CB properties and map them to AL instead.\n"
    "    // as we don't support any EBDIC based OS'es, NL is ignored and mapped to AL as well.\n"
    "    enum LineBreakClass {\n"
    "        LineBreak_OP, LineBreak_CL, LineBreak_QU, LineBreak_GL, LineBreak_NS,\n"
    "        LineBreak_EX, LineBreak_SY, LineBreak_IS, LineBreak_PR, LineBreak_PO,\n"
    "        LineBreak_NU, LineBreak_AL, LineBreak_ID, LineBreak_IN, LineBreak_HY,\n"
    "        LineBreak_BA, LineBreak_BB, LineBreak_B2, LineBreak_ZW, LineBreak_CM,\n"
    "        LineBreak_WJ, LineBreak_H2, LineBreak_H3, LineBreak_JL, LineBreak_JV,\n"
    "        LineBreak_JT, LineBreak_SA, LineBreak_SG,\n"
    "        LineBreak_SP, LineBreak_CR, LineBreak_LF, LineBreak_BK\n"
    "    };\n\n";

enum LineBreakClass {
    LineBreak_OP, LineBreak_CL, LineBreak_QU, LineBreak_GL, LineBreak_NS,
    LineBreak_EX, LineBreak_SY, LineBreak_IS, LineBreak_PR, LineBreak_PO,
    LineBreak_NU, LineBreak_AL, LineBreak_ID, LineBreak_IN, LineBreak_HY,
    LineBreak_BA, LineBreak_BB, LineBreak_B2, LineBreak_ZW, LineBreak_CM,
    LineBreak_WJ, LineBreak_H2, LineBreak_H3, LineBreak_JL, LineBreak_JV,
    LineBreak_JT, LineBreak_SA, LineBreak_SG,
    LineBreak_SP, LineBreak_CR, LineBreak_LF, LineBreak_BK

    , LineBreak_Unassigned
};

static QHash<QByteArray, LineBreakClass> line_break_map;

static void initLineBreak()
{
    // ### Classes XX and AI are left out and mapped to AL for now;
    // ### Class NL is ignored and mapped to AL as well.
    struct LineBreakList {
        LineBreakClass brk;
        const char *name;
    } breaks[] = {
        { LineBreak_BK, "BK" },
        { LineBreak_CR, "CR" },
        { LineBreak_LF, "LF" },
        { LineBreak_CM, "CM" },
        { LineBreak_AL, "NL" },
        { LineBreak_SG, "SG" },
        { LineBreak_WJ, "WJ" },
        { LineBreak_ZW, "ZW" },
        { LineBreak_GL, "GL" },
        { LineBreak_SP, "SP" },
        { LineBreak_B2, "B2" },
        { LineBreak_BA, "BA" },
        { LineBreak_BB, "BB" },
        { LineBreak_HY, "HY" },
        { LineBreak_AL, "CB" }, // ###
        { LineBreak_CL, "CL" },
        { LineBreak_EX, "EX" },
        { LineBreak_IN, "IN" },
        { LineBreak_NS, "NS" },
        { LineBreak_OP, "OP" },
        { LineBreak_QU, "QU" },
        { LineBreak_IS, "IS" },
        { LineBreak_NU, "NU" },
        { LineBreak_PO, "PO" },
        { LineBreak_PR, "PR" },
        { LineBreak_SY, "SY" },
        { LineBreak_AL, "AI" },
        { LineBreak_AL, "AL" },
        { LineBreak_H2, "H2" },
        { LineBreak_H3, "H3" },
        { LineBreak_ID, "ID" },
        { LineBreak_JL, "JL" },
        { LineBreak_JV, "JV" },
        { LineBreak_JT, "JT" },
        { LineBreak_SA, "SA" },
        { LineBreak_AL, "XX" },
        { LineBreak_Unassigned, 0 }
    };
    LineBreakList *d = breaks;
    while (d->name) {
        line_break_map.insert(d->name, d->brk);
        ++d;
    }
}


// Keep this one in sync with the code in createPropertyInfo
static const char *property_string =
    "    struct Properties {\n"
    "        ushort category         : 8; /* 5 needed */\n"
    "        ushort line_break_class : 8; /* 6 needed */\n"
    "        ushort direction        : 8; /* 5 needed */\n"
    "        ushort combiningClass   : 8;\n"
    "        ushort joining          : 2;\n"
    "        signed short digitValue : 6; /* 5 needed */\n"
    "        ushort unicodeVersion   : 4;\n"
    "        ushort lowerCaseSpecial : 1;\n"
    "        ushort upperCaseSpecial : 1;\n"
    "        ushort titleCaseSpecial : 1;\n"
    "        ushort caseFoldSpecial  : 1; /* currently unused */\n"
    "        signed short mirrorDiff    : 16;\n"
    "        signed short lowerCaseDiff : 16;\n"
    "        signed short upperCaseDiff : 16;\n"
    "        signed short titleCaseDiff : 16;\n"
    "        signed short caseFoldDiff  : 16;\n"
    "        ushort graphemeBreak    : 8; /* 4 needed */\n"
    "        ushort wordBreak        : 8; /* 4 needed */\n"
    "        ushort sentenceBreak    : 8; /* 4 needed */\n"
    "    };\n"
    "    Q_CORE_EXPORT const Properties * QT_FASTCALL properties(uint ucs4);\n"
    "    Q_CORE_EXPORT const Properties * QT_FASTCALL properties(ushort ucs2);\n";

static const char *methods =
    "    Q_CORE_EXPORT QUnicodeTables::LineBreakClass QT_FASTCALL lineBreakClass(uint ucs4);\n"
    "    inline int lineBreakClass(const QChar &ch)\n"
    "    { return lineBreakClass(ch.unicode()); }\n"
    "\n"
    "    Q_CORE_EXPORT int QT_FASTCALL script(uint ucs4);\n"
    "    inline int script(const QChar &ch)\n"
    "    { return script(ch.unicode()); }\n\n";

static const int SizeOfPropertiesStruct = 20;

struct PropertyFlags {
    bool operator ==(const PropertyFlags &o) {
        return (combiningClass == o.combiningClass
                && category == o.category
                && direction == o.direction
                && joining == o.joining
                && age == o.age
                && digitValue == o.digitValue
                && line_break_class == o.line_break_class
                && mirrorDiff == o.mirrorDiff
                && lowerCaseDiff == o.lowerCaseDiff
                && upperCaseDiff == o.upperCaseDiff
                && titleCaseDiff == o.titleCaseDiff
                && caseFoldDiff == o.caseFoldDiff
                && lowerCaseSpecial == o.lowerCaseSpecial
                && upperCaseSpecial == o.upperCaseSpecial
                && titleCaseSpecial == o.titleCaseSpecial
                && caseFoldSpecial == o.caseFoldSpecial
                && graphemeBreak == o.graphemeBreak
                && wordBreak == o.wordBreak
                && sentenceBreak == o.sentenceBreak
            );
    }
    // from UnicodeData.txt
    uchar combiningClass : 8;
    QChar::Category category : 5;
    QChar::Direction direction : 5;
    // from ArabicShaping.txt
    QChar::Joining joining : 2;
    // from DerivedAge.txt
    QChar::UnicodeVersion age : 4;
    int digitValue;
    LineBreakClass line_break_class;

    int mirrorDiff : 16;

    int lowerCaseDiff;
    int upperCaseDiff;
    int titleCaseDiff;
    int caseFoldDiff;
    bool lowerCaseSpecial;
    bool upperCaseSpecial;
    bool titleCaseSpecial;
    bool caseFoldSpecial;
    GraphemeBreak graphemeBreak;
    WordBreak wordBreak;
    SentenceBreak sentenceBreak;
};


static QList<int> specialCaseMap;
static int specialCaseMaxLen = 0;

static int appendToSpecialCaseMap(const QList<int> &map)
{
    QList<int> utf16map;
    for (int i = 0; i < map.size(); ++i) {
        int val = map.at(i);
        if (val >= 0x10000) {
            utf16map << QChar::highSurrogate(val);
            utf16map << QChar::lowSurrogate(val);
        } else {
            utf16map << val;
        }
    }
    specialCaseMaxLen = qMax(specialCaseMaxLen, utf16map.size());
    utf16map << 0;

    for (int i = 0; i < specialCaseMap.size() - utf16map.size() + 1; ++i) {
        int j;
        for (j = 0; j < utf16map.size(); ++j) {
            if (specialCaseMap.at(i+j) != utf16map.at(j))
                break;
        }
        if (j == utf16map.size())
            return i;
    }

    int pos = specialCaseMap.size();
    specialCaseMap << utf16map;
    return pos;
}

struct UnicodeData {
    UnicodeData(int codepoint = 0) {
        p.category = QChar::Other_NotAssigned; // Cn
        p.combiningClass = 0;

        p.direction = QChar::DirL;
        // DerivedBidiClass.txt
        // DirR for:  U+0590..U+05FF, U+07C0..U+08FF, U+FB1D..U+FB4F, U+10800..U+10FFF
        if ((codepoint >= 0x590 && codepoint <= 0x5ff)
            || (codepoint >= 0x7c0 && codepoint <= 0x8ff)
            || (codepoint >= 0xfb1d && codepoint <= 0xfb4f)
            || (codepoint >= 0x10800 && codepoint <= 0x10fff)) {
            p.direction = QChar::DirR;
        }
        // DirAL for:  U+0600..U+07BF, U+FB50..U+FDFF, U+FE70..U+FEFF
        //             minus noncharacter code points (intersects with U+FDD0..U+FDEF)
        if ((codepoint >= 0x600 && codepoint <= 0x7bf)
            || (codepoint >= 0xfb50 && codepoint <= 0xfdcf)
            || (codepoint >= 0xfdf0 && codepoint <= 0xfdff)
            || (codepoint >= 0xfe70 && codepoint <= 0xfeff)) {
            p.direction = QChar::DirAL;
        }

        mirroredChar = 0;
        decompositionType = QChar::NoDecomposition;
        p.joining = QChar::OtherJoining;
        p.age = QChar::Unicode_Unassigned;
        p.mirrorDiff = 0;
        p.digitValue = -1;
        p.line_break_class = LineBreak_AL; // XX -> AL
        p.lowerCaseDiff = 0;
        p.upperCaseDiff = 0;
        p.titleCaseDiff = 0;
        p.caseFoldDiff = 0;
        p.lowerCaseSpecial = 0;
        p.upperCaseSpecial = 0;
        p.titleCaseSpecial = 0;
        p.caseFoldSpecial = 0;
        p.graphemeBreak = GraphemeBreakOther;
        p.wordBreak = WordBreakOther;
        p.sentenceBreak = SentenceBreakOther;
        propertyIndex = -1;
        excludedComposition = false;
    }
    PropertyFlags p;

    // from UnicodeData.txt
    QChar::Decomposition decompositionType;
    QList<int> decomposition;

    QList<int> specialFolding;

    // from BidiMirroring.txt
    int mirroredChar;

    // DerivedNormalizationProps.txt
    bool excludedComposition;

    // computed position of unicode property set
    int propertyIndex;
};

enum UniDataFields {
    UD_Value,
    UD_Name,
    UD_Category,
    UD_CombiningClass,
    UD_BidiCategory,
    UD_Decomposition,
    UD_DecimalDigitValue,
    UD_DigitValue,
    UD_NumericValue,
    UD_Mirrored,
    UD_OldName,
    UD_Comment,
    UD_UpperCase,
    UD_LowerCase,
    UD_TitleCase
};


static QHash<QByteArray, QChar::Category> categoryMap;

static void initCategoryMap()
{
    struct Cat {
        QChar::Category cat;
        const char *name;
    } categories[] = {
        { QChar::Mark_NonSpacing,          "Mn" },
        { QChar::Mark_SpacingCombining,    "Mc" },
        { QChar::Mark_Enclosing,           "Me" },

        { QChar::Number_DecimalDigit,      "Nd" },
        { QChar::Number_Letter,            "Nl" },
        { QChar::Number_Other,             "No" },

        { QChar::Separator_Space,          "Zs" },
        { QChar::Separator_Line,           "Zl" },
        { QChar::Separator_Paragraph,      "Zp" },

        { QChar::Other_Control,            "Cc" },
        { QChar::Other_Format,             "Cf" },
        { QChar::Other_Surrogate,          "Cs" },
        { QChar::Other_PrivateUse,         "Co" },
        { QChar::Other_NotAssigned,        "Cn" },

        { QChar::Letter_Uppercase,         "Lu" },
        { QChar::Letter_Lowercase,         "Ll" },
        { QChar::Letter_Titlecase,         "Lt" },
        { QChar::Letter_Modifier,          "Lm" },
        { QChar::Letter_Other,             "Lo" },

        { QChar::Punctuation_Connector,    "Pc" },
        { QChar::Punctuation_Dash,         "Pd" },
        { QChar::Punctuation_Open,         "Ps" },
        { QChar::Punctuation_Close,        "Pe" },
        { QChar::Punctuation_InitialQuote, "Pi" },
        { QChar::Punctuation_FinalQuote,   "Pf" },
        { QChar::Punctuation_Other,        "Po" },

        { QChar::Symbol_Math,              "Sm" },
        { QChar::Symbol_Currency,          "Sc" },
        { QChar::Symbol_Modifier,          "Sk" },
        { QChar::Symbol_Other,             "So" },
        { QChar::NoCategory, 0 }
    };
    Cat *c = categories;
    while (c->name) {
        categoryMap.insert(c->name, c->cat);
        ++c;
    }
}


static QHash<QByteArray, QChar::Direction> directionMap;

static void initDirectionMap()
{
    struct Dir {
        QChar::Direction dir;
        const char *name;
    } directions[] = {
        { QChar::DirL, "L" },
        { QChar::DirR, "R" },
        { QChar::DirEN, "EN" },
        { QChar::DirES, "ES" },
        { QChar::DirET, "ET" },
        { QChar::DirAN, "AN" },
        { QChar::DirCS, "CS" },
        { QChar::DirB, "B" },
        { QChar::DirS, "S" },
        { QChar::DirWS, "WS" },
        { QChar::DirON, "ON" },
        { QChar::DirLRE, "LRE" },
        { QChar::DirLRO, "LRO" },
        { QChar::DirAL, "AL" },
        { QChar::DirRLE, "RLE" },
        { QChar::DirRLO, "RLO" },
        { QChar::DirPDF, "PDF" },
        { QChar::DirNSM, "NSM" },
        { QChar::DirBN, "BN" },
        { QChar::DirL, 0 }
    };
    Dir *d = directions;
    while (d->name) {
        directionMap.insert(d->name, d->dir);
        ++d;
    }
}


static QHash<QByteArray, QChar::Decomposition> decompositionMap;

static void initDecompositionMap()
{
    struct Dec {
        QChar::Decomposition dec;
        const char *name;
    } decompositions[] = {
        { QChar::Canonical, "<canonical>" },
        { QChar::Font, "<font>" },
        { QChar::NoBreak, "<noBreak>" },
        { QChar::Initial, "<initial>" },
        { QChar::Medial, "<medial>" },
        { QChar::Final, "<final>" },
        { QChar::Isolated, "<isolated>" },
        { QChar::Circle, "<circle>" },
        { QChar::Super, "<super>" },
        { QChar::Sub, "<sub>" },
        { QChar::Vertical, "<vertical>" },
        { QChar::Wide, "<wide>" },
        { QChar::Narrow, "<narrow>" },
        { QChar::Small, "<small>" },
        { QChar::Square, "<square>" },
        { QChar::Compat, "<compat>" },
        { QChar::Fraction, "<fraction>" },
        { QChar::NoDecomposition, 0 }
    };
    Dec *d = decompositions;
    while (d->name) {
        decompositionMap.insert(d->name, d->dec);
        ++d;
    }
}


static QHash<int, UnicodeData> unicodeData;
static QList<PropertyFlags> uniqueProperties;


static QHash<int, int> decompositionLength;
static int highestComposedCharacter = 0;
static int numLigatures = 0;
static int highestLigature = 0;

struct Ligature {
    ushort u1;
    ushort u2;
    ushort ligature;
};
// we need them sorted after the first component for fast lookup
bool operator < (const Ligature &l1, const Ligature &l2)
{ return l1.u1 < l2.u1; }

static QHash<ushort, QList<Ligature> > ligatureHashes;

static QHash<int, int> combiningClassUsage;

static int maxLowerCaseDiff = 0;
static int maxUpperCaseDiff = 0;
static int maxTitleCaseDiff = 0;

static void readUnicodeData()
{
    QFile f("data/UnicodeData.txt");
    if (!f.exists())
        qFatal("Couldn't find UnicodeData.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.truncate(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        if (line.isEmpty())
            continue;

        QList<QByteArray> properties = line.split(';');
        bool ok;
        int codepoint = properties[UD_Value].toInt(&ok, 16);
        Q_ASSERT(ok);
        Q_ASSERT(codepoint <= LAST_CODEPOINT);
        int lastCodepoint = codepoint;

        QByteArray name = properties[UD_Name];
        if (name.startsWith('<') && name.contains("First")) {
            QByteArray nextLine;
            nextLine.resize(1024);
            f.readLine(nextLine.data(), 1024);
            QList<QByteArray> properties = nextLine.split(';');
            Q_ASSERT(properties[UD_Name].startsWith('<') && properties[UD_Name].contains("Last"));
            lastCodepoint = properties[UD_Value].toInt(&ok, 16);
            Q_ASSERT(ok);
            Q_ASSERT(lastCodepoint <= LAST_CODEPOINT);
        }

        UnicodeData data(codepoint);
        data.p.category = categoryMap.value(properties[UD_Category], QChar::NoCategory);
        if (data.p.category == QChar::NoCategory)
            qFatal("unassigned char category: %s", properties[UD_Category].constData());

        data.p.combiningClass = properties[UD_CombiningClass].toInt();
        if (!combiningClassUsage.contains(data.p.combiningClass))
            combiningClassUsage[data.p.combiningClass] = 1;
        else
            ++combiningClassUsage[data.p.combiningClass];

        data.p.direction = directionMap.value(properties[UD_BidiCategory], data.p.direction);

        if (!properties[UD_UpperCase].isEmpty()) {
            int upperCase = properties[UD_UpperCase].toInt(&ok, 16);
            Q_ASSERT(ok);
            int diff = upperCase - codepoint;
            if (qAbs(diff) >= (1<<14))
                qWarning() << "upperCaseDiff exceeded (" << hex << codepoint << "->" << upperCase << ")";
            data.p.upperCaseDiff = diff;
            maxUpperCaseDiff = qMax(maxUpperCaseDiff, qAbs(diff));
            if (codepoint >= 0x10000 || upperCase >= 0x10000) {
                // if the conditions below doesn't hold anymore we need to modify our upper casing code
                Q_ASSERT(QChar::highSurrogate(codepoint) == QChar::highSurrogate(upperCase));
                Q_ASSERT(QChar::lowSurrogate(codepoint) + diff == QChar::lowSurrogate(upperCase));
            }
        }
        if (!properties[UD_LowerCase].isEmpty()) {
            int lowerCase = properties[UD_LowerCase].toInt(&ok, 16);
            Q_ASSERT(ok);
            int diff = lowerCase - codepoint;
            if (qAbs(diff) >= (1<<14))
                qWarning() << "lowerCaseDiff exceeded (" << hex << codepoint << "->" << lowerCase << ")";
            data.p.lowerCaseDiff = diff;
            maxLowerCaseDiff = qMax(maxLowerCaseDiff, qAbs(diff));
            if (codepoint >= 0x10000 || lowerCase >= 0x10000) {
                // if the conditions below doesn't hold anymore we need to modify our lower casing code
                Q_ASSERT(QChar::highSurrogate(codepoint) == QChar::highSurrogate(lowerCase));
                Q_ASSERT(QChar::lowSurrogate(codepoint) + diff == QChar::lowSurrogate(lowerCase));
            }
        }
        // we want toTitleCase to map to ToUpper in case we don't have any titlecase.
        if (properties[UD_TitleCase].isEmpty())
            properties[UD_TitleCase] = properties[UD_UpperCase];
        if (!properties[UD_TitleCase].isEmpty()) {
            int titleCase = properties[UD_TitleCase].toInt(&ok, 16);
            Q_ASSERT(ok);
            int diff = titleCase - codepoint;
            if (qAbs(diff) >= (1<<14))
                qWarning() << "titleCaseDiff exceeded (" << hex << codepoint << "->" << titleCase << ")";
            data.p.titleCaseDiff = diff;
            maxTitleCaseDiff = qMax(maxTitleCaseDiff, qAbs(diff));
            if (codepoint >= 0x10000 || titleCase >= 0x10000) {
                // if the conditions below doesn't hold anymore we need to modify our title casing code
                Q_ASSERT(QChar::highSurrogate(codepoint) == QChar::highSurrogate(titleCase));
                Q_ASSERT(QChar::lowSurrogate(codepoint) + diff == QChar::lowSurrogate(titleCase));
            }
        }

        if (!properties[UD_DigitValue].isEmpty())
            data.p.digitValue = properties[UD_DigitValue].toInt();

        // decompositition
        QByteArray decomposition = properties[UD_Decomposition];
        if (!decomposition.isEmpty()) {
            highestComposedCharacter = qMax(highestComposedCharacter, codepoint);
            QList<QByteArray> d = decomposition.split(' ');
            if (d[0].contains('<')) {
                data.decompositionType = decompositionMap.value(d[0], QChar::NoDecomposition);
                if (data.decompositionType == QChar::NoDecomposition)
                    qFatal("unassigned decomposition type: %s", d[0].constData());
                d.takeFirst();
            } else {
                data.decompositionType = QChar::Canonical;
            }
            for (int i = 0; i < d.size(); ++i) {
                data.decomposition.append(d[i].toInt(&ok, 16));
                Q_ASSERT(ok);
            }
            if (!decompositionLength.contains(data.decomposition.size()))
                decompositionLength[data.decomposition.size()] = 1;
            else
                ++decompositionLength[data.decomposition.size()];
        }

        for (int i = codepoint; i <= lastCodepoint; ++i)
            unicodeData.insert(i, data);
    }

}

static int maxMirroredDiff = 0;

static void readBidiMirroring()
{
    QFile f("data/BidiMirroring.txt");
    if (!f.exists())
        qFatal("Couldn't find BidiMirroring.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);

        if (line.isEmpty())
            continue;
        line = line.replace(" ", "");

        QList<QByteArray> pair = line.split(';');
        Q_ASSERT(pair.size() == 2);

        bool ok;
        int codepoint = pair[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int mirror = pair[1].toInt(&ok, 16);
        Q_ASSERT(ok);

        UnicodeData d = unicodeData.value(codepoint, UnicodeData(codepoint));
        d.mirroredChar = mirror;
        d.p.mirrorDiff = d.mirroredChar - codepoint;
        maxMirroredDiff = qMax(maxMirroredDiff, qAbs(d.p.mirrorDiff));
        unicodeData.insert(codepoint, d);
    }
}

static void readArabicShaping()
{
    QFile f("data/ArabicShaping.txt");
    if (!f.exists())
        qFatal("Couldn't find ArabicShaping.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line = line.trimmed();

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 4);

        bool ok;
        int codepoint = l[0].toInt(&ok, 16);
        Q_ASSERT(ok);

        Joining joining = joining_map.value(l[2].trimmed(), Joining_Unassigned);
        if (joining == Joining_Unassigned)
            qFatal("unassigned or unhandled joining value: %s", l[2].constData());

        if (joining == Joining_Left) {
            // There are currently no characters of joining type Left_Joining defined in Unicode.
            qFatal("%x: joining type '%s' was met; the current implementation needs to be revised!", codepoint, l[2].constData());
        }

        UnicodeData d = unicodeData.value(codepoint, UnicodeData(codepoint));
        if (joining == Joining_Right)
            d.p.joining = QChar::Right;
        else if (joining == Joining_Dual)
            d.p.joining = QChar::Dual;
        else if (joining == Joining_Causing)
            d.p.joining = QChar::Center;
        else
            d.p.joining = QChar::OtherJoining;
        unicodeData.insert(codepoint, d);
    }
}

static void readDerivedAge()
{
    QFile f("data/DerivedAge.txt");
    if (!f.exists())
        qFatal("Couldn't find DerivedAge.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        QChar::UnicodeVersion age = age_map.value(l[1].trimmed(), QChar::Unicode_Unassigned);
        //qDebug() << hex << from << ".." << to << ba << age;
        if (age == QChar::Unicode_Unassigned)
            qFatal("unassigned or unhandled age value: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData d = unicodeData.value(codepoint, UnicodeData(codepoint));
            d.p.age = age;
            unicodeData.insert(codepoint, d);
        }
    }
}


static void readDerivedNormalizationProps()
{
    QFile f("data/DerivedNormalizationProps.txt");
    if (!f.exists())
        qFatal("Couldn't find DerivedNormalizationProps.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);

        if (line.trimmed().isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() >= 2);

        QByteArray propName = l[1].trimmed();
        if (propName != "Full_Composition_Exclusion")
            // ###
            continue;

        QByteArray codes = l[0].trimmed();
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData d = unicodeData.value(codepoint, UnicodeData(codepoint));
            d.excludedComposition = true;
            unicodeData.insert(codepoint, d);
        }
    }

    for (int codepoint = 0; codepoint <= LAST_CODEPOINT; ++codepoint) {
        UnicodeData d = unicodeData.value(codepoint, UnicodeData(codepoint));
        if (!d.excludedComposition
            && d.decompositionType == QChar::Canonical
            && d.decomposition.size() > 1) {
            Q_ASSERT(d.decomposition.size() == 2);

            int part1 = d.decomposition.at(0);
            int part2 = d.decomposition.at(1);

            // all non-starters are listed in DerivedNormalizationProps.txt
            // and already excluded from composition
            Q_ASSERT(unicodeData.value(part1, UnicodeData(part1)).p.combiningClass == 0);

            ++numLigatures;
            highestLigature = qMax(highestLigature, part1);
            Ligature l = {(ushort)part1, (ushort)part2, (ushort)codepoint};
            ligatureHashes[part2].append(l);
        }
    }
}


struct NormalizationCorrection {
    uint codepoint;
    uint mapped;
    uint version;
};

static QByteArray createNormalizationCorrections()
{
    QFile f("data/NormalizationCorrections.txt");
    if (!f.exists())
        qFatal("Couldn't find NormalizationCorrections.txt");

    f.open(QFile::ReadOnly);

    QByteArray out;

    out += "struct NormalizationCorrection {\n"
           "    uint ucs4;\n"
           "    uint old_mapping;\n"
           "    int version;\n"
           "};\n\n"

           "static const NormalizationCorrection uc_normalization_corrections[] = {\n";

    int numCorrections = 0;
    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        Q_ASSERT(!line.contains(".."));

        QList<QByteArray> fields = line.split(';');
        Q_ASSERT(fields.size() == 4);

        NormalizationCorrection c = { 0, 0, 0 };
        bool ok;
        c.codepoint = fields.at(0).toInt(&ok, 16);
        Q_ASSERT(ok);
        c.mapped = fields.at(1).toInt(&ok, 16);
        Q_ASSERT(ok);
        if (fields.at(3) == "3.2.0")
            c.version = QChar::Unicode_3_2;
        else if (fields.at(3) == "4.0.0")
            c.version = QChar::Unicode_4_0;
        else
            qFatal("unknown unicode version in NormalizationCorrection.txt");

        out += "    { 0x" + QByteArray::number(c.codepoint, 16) + ", 0x" + QByteArray::number(c.mapped, 16)
             + ", " + QString::number(c.version) + " },\n";
        ++numCorrections;
    }

    out += "};\n\n"

           "enum { NumNormalizationCorrections = " + QByteArray::number(numCorrections) + " };\n\n";

    return out;
}


static void computeUniqueProperties()
{
    qDebug("computeUniqueProperties:");
    for (int uc = 0; uc <= LAST_CODEPOINT; ++uc) {
        UnicodeData d = unicodeData.value(uc, UnicodeData(uc));

        int index = uniqueProperties.indexOf(d.p);
        if (index == -1) {
            index = uniqueProperties.size();
            uniqueProperties.append(d.p);
        }
        d.propertyIndex = index;
        unicodeData.insert(uc, d);
    }
    qDebug("    %d unique unicode properties found", uniqueProperties.size());
}


static void readLineBreak()
{
    qDebug() << "Reading LineBreak.txt";
    QFile f("data/LineBreak.txt");
    if (!f.exists())
        qFatal("Couldn't find LineBreak.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        LineBreakClass lb = line_break_map.value(l[1], LineBreak_Unassigned);
        if (lb == LineBreak_Unassigned)
            qFatal("unassigned line break class: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData d = unicodeData.value(codepoint, UnicodeData(codepoint));
            d.p.line_break_class = lb;
            unicodeData.insert(codepoint, d);
        }
    }
}


static void readSpecialCasing()
{
    qDebug() << "Reading SpecialCasing.txt";
    QFile f("data/SpecialCasing.txt");
    if (!f.exists())
        qFatal("Couldn't find SpecialCasing.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');

        QByteArray condition = l.size() < 5 ? QByteArray() : l[4].trimmed();
        if (!condition.isEmpty())
            // #####
            continue;

        bool ok;
        int codepoint = l[0].trimmed().toInt(&ok, 16);
        Q_ASSERT(ok);

        // if the condition below doesn't hold anymore we need to modify our
        // lower/upper/title casing code and case folding code
        Q_ASSERT(codepoint < 0x10000);

//         qDebug() << "codepoint" << hex << codepoint;
//         qDebug() << line;

        QList<QByteArray> lower = l[1].trimmed().split(' ');
        QList<int> lowerMap;
        for (int i = 0; i < lower.size(); ++i) {
            bool ok;
            lowerMap.append(lower.at(i).toInt(&ok, 16));
            Q_ASSERT(ok);
        }

        QList<QByteArray> title = l[2].trimmed().split(' ');
        QList<int> titleMap;
        for (int i = 0; i < title.size(); ++i) {
            bool ok;
            titleMap.append(title.at(i).toInt(&ok, 16));
            Q_ASSERT(ok);
        }

        QList<QByteArray> upper = l[3].trimmed().split(' ');
        QList<int> upperMap;
        for (int i = 0; i < upper.size(); ++i) {
            bool ok;
            upperMap.append(upper.at(i).toInt(&ok, 16));
            Q_ASSERT(ok);
        }


        UnicodeData ud = unicodeData.value(codepoint, UnicodeData(codepoint));

        Q_ASSERT(lowerMap.size() > 1 || lowerMap.at(0) == codepoint + ud.p.lowerCaseDiff);
        Q_ASSERT(titleMap.size() > 1 || titleMap.at(0) == codepoint + ud.p.titleCaseDiff);
        Q_ASSERT(upperMap.size() > 1 || upperMap.at(0) == codepoint + ud.p.upperCaseDiff);

        if (lowerMap.size() > 1) {
            ud.p.lowerCaseSpecial = true;
            ud.p.lowerCaseDiff = appendToSpecialCaseMap(lowerMap);
        }
        if (titleMap.size() > 1) {
            ud.p.titleCaseSpecial = true;
            ud.p.titleCaseDiff = appendToSpecialCaseMap(titleMap);
        }
        if (upperMap.size() > 1) {
            ud.p.upperCaseSpecial = true;
            ud.p.upperCaseDiff = appendToSpecialCaseMap(upperMap);;
        }

        unicodeData.insert(codepoint, ud);
    }
}

static int maxCaseFoldDiff = 0;

static void readCaseFolding()
{
    qDebug() << "Reading CaseFolding.txt";
    QFile f("data/CaseFolding.txt");
    if (!f.exists())
        qFatal("Couldn't find CaseFolding.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');

        bool ok;
        int codepoint = l[0].trimmed().toInt(&ok, 16);
        Q_ASSERT(ok);


        l[1] = l[1].trimmed();
        if (l[1] == "F" || l[1] == "T")
            continue;

//         qDebug() << "codepoint" << hex << codepoint;
//         qDebug() << line;
        QList<QByteArray> fold = l[2].trimmed().split(' ');
        QList<int> foldMap;
        for (int i = 0; i < fold.size(); ++i) {
            bool ok;
            foldMap.append(fold.at(i).toInt(&ok, 16));
            Q_ASSERT(ok);
        }

        UnicodeData ud = unicodeData.value(codepoint, UnicodeData(codepoint));
        if (foldMap.size() == 1) {
            int caseFolded = foldMap.at(0);
            int diff = caseFolded - codepoint;
            if (qAbs(diff) >= (1<<14))
                qWarning() << "caseFoldDiff exceeded (" << hex << codepoint << "->" << caseFolded << ")";
            ud.p.caseFoldDiff = diff;
            maxCaseFoldDiff = qMax(maxCaseFoldDiff, qAbs(diff));
            if (codepoint >= 0x10000 || caseFolded >= 0x10000) {
                // if the conditions below doesn't hold anymore we need to modify our case folding code
                Q_ASSERT(QChar::highSurrogate(codepoint) == QChar::highSurrogate(caseFolded));
                Q_ASSERT(QChar::lowSurrogate(codepoint) + diff == QChar::lowSurrogate(caseFolded));
            }
            if (caseFolded != codepoint + ud.p.lowerCaseDiff)
                qDebug() << hex << codepoint;
        } else {
            qFatal("we currently don't support full case foldings");
//             qDebug() << "special" << hex << foldMap;
            ud.p.caseFoldSpecial = true;
            ud.p.caseFoldDiff = appendToSpecialCaseMap(foldMap);
        }
        unicodeData.insert(codepoint, ud);
    }
}

static void readGraphemeBreak()
{
    qDebug() << "Reading GraphemeBreakProperty.txt";
    QFile f("data/GraphemeBreakProperty.txt");
    if (!f.exists())
        qFatal("Couldn't find GraphemeBreakProperty.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        GraphemeBreak brk = grapheme_break_map.value(l[1], GraphemeBreak_Unassigned);
        if (brk == GraphemeBreak_Unassigned)
            qFatal("unassigned grapheme break class: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData ud = unicodeData.value(codepoint, UnicodeData(codepoint));
            ud.p.graphemeBreak = brk;
            unicodeData.insert(codepoint, ud);
        }
    }
}

static void readWordBreak()
{
    qDebug() << "Reading WordBreakProperty.txt";
    QFile f("data/WordBreakProperty.txt");
    if (!f.exists())
        qFatal("Couldn't find WordBreakProperty.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        WordBreak brk = word_break_map.value(l[1], WordBreak_Unassigned);
        if (brk == WordBreak_Unassigned)
            qFatal("unassigned word break class: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData ud = unicodeData.value(codepoint, UnicodeData(codepoint));
            ud.p.wordBreak = brk;
            unicodeData.insert(codepoint, ud);
        }
    }
}

static void readSentenceBreak()
{
    qDebug() << "Reading SentenceBreakProperty.txt";
    QFile f("data/SentenceBreakProperty.txt");
    if (!f.exists())
        qFatal("Couldn't find SentenceBreakProperty.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line;
        line.resize(1024);
        int len = f.readLine(line.data(), 1024);
        line.resize(len-1);

        int comment = line.indexOf('#');
        if (comment >= 0)
            line = line.left(comment);
        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        QList<QByteArray> l = line.split(';');
        Q_ASSERT(l.size() == 2);

        QByteArray codes = l[0];
        codes.replace("..", ".");
        QList<QByteArray> cl = codes.split('.');

        bool ok;
        int from = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int to = from;
        if (cl.size() == 2) {
            to = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        SentenceBreak brk = sentence_break_map.value(l[1], SentenceBreak_Unassigned);
        if (brk == SentenceBreak_Unassigned)
            qFatal("unassigned sentence break class: %s", l[1].constData());

        for (int codepoint = from; codepoint <= to; ++codepoint) {
            UnicodeData ud = unicodeData.value(codepoint, UnicodeData(codepoint));
            ud.p.sentenceBreak = brk;
            unicodeData.insert(codepoint, ud);
        }
    }
}

#if 0
// this piece of code does full case folding and comparison. We currently
// don't use it, since this gives lots of issues with things as case insensitive
// search and replace.
static inline void foldCase(uint ch, ushort *out)
{
    const QUnicodeTables::Properties *p = qGetProp(ch);
    if (!p->caseFoldSpecial) {
        *(out++) = ch + p->caseFoldDiff;
    } else {
        const ushort *folded = specialCaseMap + p->caseFoldDiff;
        while (*folded)
            *out++ = *folded++;
    }
    *out = 0;
}

static int ucstricmp(const ushort *a, const ushort *ae, const ushort *b, const ushort *be)
{
    if (a == b)
        return 0;
    if (a == 0)
        return 1;
    if (b == 0)
        return -1;

    while (a != ae && b != be) {
        const QUnicodeTables::Properties *pa = qGetProp(*a);
        const QUnicodeTables::Properties *pb = qGetProp(*b);
        if (pa->caseFoldSpecial | pb->caseFoldSpecial)
            goto special;
            int diff = (int)(*a + pa->caseFoldDiff) - (int)(*b + pb->caseFoldDiff);
        if ((diff))
            return diff;
        ++a;
        ++b;
        }
    }
    if (a == ae) {
        if (b == be)
            return 0;
        return -1;
    }
    return 1;
special:
    ushort abuf[SPECIAL_CASE_MAX_LEN + 1];
    ushort bbuf[SPECIAL_CASE_MAX_LEN + 1];
    abuf[0] = bbuf[0] = 0;
    ushort *ap = abuf;
    ushort *bp = bbuf;
    while (1) {
        if (!*ap) {
            if (a == ae) {
                if (!*bp && b == be)
                    return 0;
                return -1;
            }
            foldCase(*(a++), abuf);
            ap = abuf;
        }
        if (!*bp) {
            if (b == be)
                return 1;
            foldCase(*(b++), bbuf);
            bp = bbuf;
        }
        if (*ap != *bp)
            return (int)*ap - (int)*bp;
        ++ap;
        ++bp;
    }
}


static int ucstricmp(const ushort *a, const ushort *ae, const uchar *b)
{
    if (a == 0)
        return 1;
    if (b == 0)
        return -1;

    while (a != ae && *b) {
        const QUnicodeTables::Properties *pa = qGetProp(*a);
        const QUnicodeTables::Properties *pb = qGetProp((ushort)*b);
        if (pa->caseFoldSpecial | pb->caseFoldSpecial)
            goto special;
        int diff = (int)(*a + pa->caseFoldDiff) - (int)(*b + pb->caseFoldDiff);
        if ((diff))
            return diff;
        ++a;
        ++b;
    }
    if (a == ae) {
        if (!*b)
            return 0;
        return -1;
    }
    return 1;

special:
    ushort abuf[SPECIAL_CASE_MAX_LEN + 1];
    ushort bbuf[SPECIAL_CASE_MAX_LEN + 1];
    abuf[0] = bbuf[0] = 0;
    ushort *ap = abuf;
    ushort *bp = bbuf;
    while (1) {
        if (!*ap) {
            if (a == ae) {
                if (!*bp && !*b)
                    return 0;
                return -1;
            }
            foldCase(*(a++), abuf);
            ap = abuf;
        }
        if (!*bp) {
            if (!*b)
                return 1;
            foldCase(*(b++), bbuf);
            bp = bbuf;
        }
        if (*ap != *bp)
            return (int)*ap - (int)*bp;
        ++ap;
        ++bp;
    }
}
#endif

#if 0
static QList<QByteArray> blockNames;
struct BlockInfo
{
    int blockIndex;
    int firstCodePoint;
    int lastCodePoint;
};
static QList<BlockInfo> blockInfoList;

static void readBlocks()
{
    QFile f("data/Blocks.txt");
    if (!f.exists())
        qFatal("Couldn't find Blocks.txt");

    f.open(QFile::ReadOnly);

    while (!f.atEnd()) {
        QByteArray line = f.readLine();
        line.resize(line.size() - 1);

        int comment = line.indexOf("#");
        if (comment >= 0)
            line = line.left(comment);

        line.replace(" ", "");

        if (line.isEmpty())
            continue;

        int semicolon = line.indexOf(';');
        Q_ASSERT(semicolon >= 0);
        QByteArray codePoints = line.left(semicolon);
        QByteArray blockName = line.mid(semicolon + 1);

        int blockIndex = blockNames.indexOf(blockName);
        if (blockIndex == -1) {
            blockIndex = blockNames.size();
            blockNames.append(blockName);
        }

        codePoints.replace("..", ".");
        QList<QByteArray> cl = codePoints.split('.');

        bool ok;
        int first = cl[0].toInt(&ok, 16);
        Q_ASSERT(ok);
        int last = first;
        if (cl.size() == 2) {
            last = cl[1].toInt(&ok, 16);
            Q_ASSERT(ok);
        }

        BlockInfo blockInfo = { blockIndex, first, last };
        blockInfoList.append(blockInfo);
    }
}
#endif

static QList<QByteArray> scriptNames;
static QHash<int, int> scriptAssignment;
static QHash<int, int> scriptHash;

struct ExtraBlock {
    int block;
    QVector<int> vector;
};

static QList<ExtraBlock> extraBlockList;


static void readScripts()
{
    scriptNames.append("Common");

    static const char *files[] = {
        "data/ScriptsInitial.txt",
        "data/Scripts.txt",
        "data/ScriptsCorrections.txt"
    };
    enum { fileCount = sizeof(files) / sizeof(const char *) };

    for (int i = 0; i < fileCount; ++i) {
        QFile f(files[i]);
        if (!f.exists())
            qFatal("Couldn't find %s", files[i]);

        f.open(QFile::ReadOnly);

        while (!f.atEnd()) {
            QByteArray line = f.readLine();
            line.resize(line.size() - 1);

            int comment = line.indexOf("#");
            if (comment >= 0)
                line = line.left(comment);

            line.replace(" ", "");
            line.replace("_", "");

            if (line.isEmpty())
                continue;

            int semicolon = line.indexOf(';');
            Q_ASSERT(semicolon >= 0);
            QByteArray codePoints = line.left(semicolon);
            QByteArray scriptName = line.mid(semicolon + 1);

            int scriptIndex = scriptNames.indexOf(scriptName);
            if (scriptIndex == -1) {
                scriptIndex = scriptNames.size();
                scriptNames.append(scriptName);
            }

            codePoints.replace("..", ".");
            QList<QByteArray> cl = codePoints.split('.');

            bool ok;
            int first = cl[0].toInt(&ok, 16);
            Q_ASSERT(ok);
            int last = first;
            if (cl.size() == 2) {
                last = cl[1].toInt(&ok, 16);
                Q_ASSERT(ok);
            }

            for (int i = first; i <= last; ++i)
                scriptAssignment[i] = scriptIndex;
        }
    }
}


static int scriptSentinel = 0;

QByteArray createScriptEnumDeclaration()
{
    static const char *specialScripts[] = {
        "Common",
        "Arabic",
        "Armenian",
        "Bengali",
        "Cyrillic",
        "Devanagari",
        "Georgian",
        "Greek",
        "Gujarati",
        "Gurmukhi",
        "Hangul",
        "Hebrew",
        "Kannada",
        "Khmer",
        "Lao",
        "Malayalam",
        "Myanmar",
        "Nko",
        "Ogham",
        "Oriya",
        "Runic",
        "Sinhala",
        "Syriac",
        "Tamil",
        "Telugu",
        "Thaana",
        "Thai",
        "Tibetan",
        "Inherited"
    };
    const int specialScriptsCount = sizeof(specialScripts) / sizeof(const char *);

    // generate script enum
    QByteArray declaration;

    declaration += "    // See http://www.unicode.org/reports/tr24/tr24-5.html\n";
    declaration += "    enum Script {\n        Common";

    int uniqueScripts = 1; // Common

    // output the ones with special processing first
    for (int i = 1; i < scriptNames.size(); ++i) {
        QByteArray scriptName = scriptNames.at(i);
        // does the script require special processing?
        bool special = false;
        for (int s = 0; s < specialScriptsCount; ++s) {
            if (scriptName == specialScripts[s]) {
                special = true;
                break;
            }
        }
        if (!special) {
            scriptHash[i] = 0; // alias for 'Common'
            continue;
        } else {
            ++uniqueScripts;
            scriptHash[i] = i;
        }

        if (scriptName != "Inherited") {
            declaration += ",\n        ";
            declaration += scriptName;
        }
    }
    declaration += ",\n        Inherited";
    declaration += ",\n        ScriptCount = Inherited";

    // output the ones that are an alias for 'Common'
    for (int i = 1; i < scriptNames.size(); ++i) {
        if (scriptHash.value(i) != 0)
            continue;
        declaration += ",\n        ";
        declaration += scriptNames.at(i);
        declaration += " = Common";
    }

    declaration += "\n    };\n";

    scriptSentinel = ((uniqueScripts + 16) / 32) * 32; // a multiple of 32
    declaration += "    enum { ScriptSentinel = ";
    declaration += QByteArray::number(scriptSentinel);
    declaration += " };\n\n";
    return declaration;
}

QByteArray createScriptTableDeclaration()
{
    Q_ASSERT(scriptSentinel > 0);

    QByteArray declaration;

    const int unicodeBlockCount = 512; // number of unicode blocks
    const int unicodeBlockSize = 128; // size of each block
    declaration = "enum { UnicodeBlockCount = ";
    declaration += QByteArray::number(unicodeBlockCount);
    declaration += " }; // number of unicode blocks\n";
    declaration += "enum { UnicodeBlockSize = ";
    declaration += QByteArray::number(unicodeBlockSize);
    declaration += " }; // size of each block\n\n";

    // script table
    declaration += "namespace QUnicodeTables {\n\nstatic const unsigned char uc_scripts[] = {\n";
    for (int i = 0; i < unicodeBlockCount; ++i) {
        int block = (((i << 7) & 0xff00) | ((i & 1) * 0x80));
        int blockAssignment[unicodeBlockSize];
        for (int x = 0; x < unicodeBlockSize; ++x) {
            int codePoint = (i << 7) | x;
            blockAssignment[x] = scriptAssignment.value(codePoint, 0);
        }
        bool allTheSame = true;
        const int originalScript = blockAssignment[0];
        const int script = scriptHash.value(originalScript);
        for (int x = 1; allTheSame && x < unicodeBlockSize; ++x) {
            const int s = scriptHash.value(blockAssignment[x]);
            if (s != script)
                allTheSame = false;
        }

        if (allTheSame) {
            declaration += "    ";
            declaration += scriptNames.value(originalScript);
            declaration += ", /* U+";
            declaration += QByteArray::number(block, 16).rightJustified(4, '0');
            declaration += '-';
            declaration += QByteArray::number(block + unicodeBlockSize - 1, 16).rightJustified(4, '0');
            declaration += " */\n";
        } else {
            const int value = extraBlockList.size() + scriptSentinel;
            const int offset = ((value - scriptSentinel) * unicodeBlockSize) + unicodeBlockCount;

            declaration += "    ";
            declaration += QByteArray::number(value);
            declaration += ", /* U+";
            declaration += QByteArray::number(block, 16).rightJustified(4, '0');
            declaration += '-';
            declaration += QByteArray::number(block + unicodeBlockSize - 1, 16).rightJustified(4, '0');
            declaration += " at offset ";
            declaration += QByteArray::number(offset);
            declaration += " */\n";

            ExtraBlock extraBlock;
            extraBlock.block = block;
            extraBlock.vector.resize(unicodeBlockSize);
            for (int x = 0; x < unicodeBlockSize; ++x)
                extraBlock.vector[x] = blockAssignment[x];

            extraBlockList.append(extraBlock);
        }
    }

    for (int i = 0; i < extraBlockList.size(); ++i) {
        const int value = i + scriptSentinel;
        const int offset = ((value - scriptSentinel) * unicodeBlockSize) + unicodeBlockCount;
        const ExtraBlock &extraBlock = extraBlockList.at(i);
        const int block = extraBlock.block;

        declaration += "\n\n    /* U+";
        declaration += QByteArray::number(block, 16).rightJustified(4, '0');
        declaration += '-';
        declaration += QByteArray::number(block + unicodeBlockSize - 1, 16).rightJustified(4, '0');
        declaration += " at offset ";
        declaration += QByteArray::number(offset);
        declaration += " */\n    ";

        for (int x = 0; x < extraBlock.vector.size(); ++x) {
            const int o = extraBlock.vector.at(x);

            declaration += scriptNames.value(o);
            if (x < extraBlock.vector.size() - 1 || i < extraBlockList.size() - 1)
                declaration += ',';
            if ((x & 7) == 7 && x < extraBlock.vector.size() - 1)
                declaration += "\n    ";
            else
                declaration += ' ';
        }
        if (declaration.endsWith(' '))
            declaration.chop(1);
    }
    declaration += "\n};\n\n} // namespace QUnicodeTables\n\n";

    declaration += 
            "Q_CORE_EXPORT int QT_FASTCALL QUnicodeTables::script(uint ucs4)\n"
            "{\n"
            "    if (ucs4 > 0xffff)\n"
            "        return Common;\n"
            "    int script = uc_scripts[ucs4 >> 7];\n"
            "    if (script < ScriptSentinel)\n"
            "        return script;\n"
            "    script = (((script - ScriptSentinel) * UnicodeBlockSize) + UnicodeBlockCount);\n"
            "    script = uc_scripts[script + (ucs4 & 0x7f)];\n"
            "    return script;\n"
            "}\n\n";

    qDebug("createScriptTableDeclaration: table size is %d bytes",
           unicodeBlockCount + (extraBlockList.size() * unicodeBlockSize));

    return declaration;
}

#if 0
static void dump(int from, int to)
{
    for (int i = from; i <= to; ++i) {
        UnicodeData d = unicodeData.value(i, UnicodeData(i));
        qDebug("0x%04x: cat=%d combining=%d dir=%d case=%x mirror=%x joining=%d age=%d",
               i, d.p.category, d.p.combiningClass, d.p.direction, d.otherCase, d.mirroredChar, d.p.joining, d.p.age);
        if (d.decompositionType != QChar::NoDecomposition) {
            qDebug("    decomposition: type=%d, length=%d, first=%x", d.decompositionType, d.decomposition.size(),
                   d.decomposition[0]);
        }
    }
    qDebug(" ");
}
#endif

struct PropertyBlock {
    PropertyBlock() { index = -1; }
    int index;
    QList<int> properties;
    bool operator==(const PropertyBlock &other)
    { return properties == other.properties; }
};

static QByteArray createPropertyInfo()
{
    qDebug("createPropertyInfo:");

    const int BMP_BLOCKSIZE = 32;
    const int BMP_SHIFT = 5;
    const int BMP_END = 0x11000;
    const int SMP_END = 0x110000;
    const int SMP_BLOCKSIZE = 256;
    const int SMP_SHIFT = 8;

    QList<PropertyBlock> blocks;
    QList<int> blockMap;

    int used = 0;

    for (int block = 0; block < BMP_END/BMP_BLOCKSIZE; ++block) {
        PropertyBlock b;
        for (int i = 0; i < BMP_BLOCKSIZE; ++i) {
            int uc = block*BMP_BLOCKSIZE + i;
            UnicodeData d = unicodeData.value(uc, UnicodeData(uc));
            b.properties.append(d.propertyIndex);
        }
        int index = blocks.indexOf(b);
        if (index == -1) {
            index = blocks.size();
            b.index = used;
            used += BMP_BLOCKSIZE;
            blocks.append(b);
        }
        blockMap.append(blocks.at(index).index);
    }

    int bmp_blocks = blocks.size();
    Q_ASSERT(blockMap.size() == BMP_END/BMP_BLOCKSIZE);

    for (int block = BMP_END/SMP_BLOCKSIZE; block < SMP_END/SMP_BLOCKSIZE; ++block) {
        PropertyBlock b;
        for (int i = 0; i < SMP_BLOCKSIZE; ++i) {
            int uc = block*SMP_BLOCKSIZE + i;
            UnicodeData d = unicodeData.value(uc, UnicodeData(uc));
            b.properties.append(d.propertyIndex);
        }
        int index = blocks.indexOf(b);
        if (index == -1) {
            index = blocks.size();
            b.index = used;
            used += SMP_BLOCKSIZE;
            blocks.append(b);
        }
        blockMap.append(blocks.at(index).index);
    }

    int bmp_block_data = bmp_blocks*BMP_BLOCKSIZE*2;
    int bmp_trie = BMP_END/BMP_BLOCKSIZE*2;
    int bmp_mem = bmp_block_data + bmp_trie;
    qDebug("    %d unique blocks in BMP.", blocks.size());
    qDebug("        block data uses: %d bytes", bmp_block_data);
    qDebug("        trie data uses : %d bytes", bmp_trie);

    int smp_block_data = (blocks.size() - bmp_blocks)*SMP_BLOCKSIZE*2;
    int smp_trie = (SMP_END-BMP_END)/SMP_BLOCKSIZE*2;
    int smp_mem = smp_block_data + smp_trie;
    qDebug("    %d unique blocks in SMP.", blocks.size()-bmp_blocks);
    qDebug("        block data uses: %d bytes", smp_block_data);
    qDebug("        trie data uses : %d bytes", smp_trie);

    qDebug("\n        properties uses : %d bytes", uniqueProperties.size() * SizeOfPropertiesStruct);
    qDebug("    memory usage: %d bytes", bmp_mem + smp_mem + uniqueProperties.size() * SizeOfPropertiesStruct);

    QByteArray out;
    out += "static const unsigned short uc_property_trie[] = {\n";

    // first write the map
    out += "    // 0 - 0x" + QByteArray::number(BMP_END, 16);
    for (int i = 0; i < BMP_END/BMP_BLOCKSIZE; ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!((i*BMP_BLOCKSIZE) % 0x1000))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n\n    // 0x" + QByteArray::number(BMP_END, 16) + " - 0x" + QByteArray::number(SMP_END, 16) + "\n";;
    for (int i = BMP_END/BMP_BLOCKSIZE; i < blockMap.size(); ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!(i % (0x10000/SMP_BLOCKSIZE)))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n";
    // write the data
    for (int i = 0; i < blocks.size(); ++i) {
        if (out.endsWith(' '))
            out.chop(1);
        out += "\n";
        const PropertyBlock &b = blocks.at(i);
        for (int j = 0; j < b.properties.size(); ++j) {
            if (!(j % 8)) {
                if (out.endsWith(' '))
                    out.chop(1);
                out += "\n    ";
            }
            out += QByteArray::number(b.properties.at(j));
            out += ", ";
        }
    }

    // we reserve one bit more than in the assert below for the sign
    Q_ASSERT(maxMirroredDiff < (1<<12));
    Q_ASSERT(maxLowerCaseDiff < (1<<14));
    Q_ASSERT(maxUpperCaseDiff < (1<<14));
    Q_ASSERT(maxTitleCaseDiff < (1<<14));
    Q_ASSERT(maxCaseFoldDiff < (1<<14));

    if (out.endsWith(' '))
        out.chop(1);
    out += "\n};\n\n"

           "#define GET_PROP_INDEX(ucs4) \\\n"
           "       (ucs4 < 0x" + QByteArray::number(BMP_END, 16) + " \\\n"
           "        ? (uc_property_trie[uc_property_trie[ucs4>>" + QByteArray::number(BMP_SHIFT) +
           "] + (ucs4 & 0x" + QByteArray::number(BMP_BLOCKSIZE-1, 16)+ ")]) \\\n"
           "        : (uc_property_trie[uc_property_trie[((ucs4 - 0x" + QByteArray::number(BMP_END, 16) +
           ")>>" + QByteArray::number(SMP_SHIFT) + ") + 0x" + QByteArray::number(BMP_END/BMP_BLOCKSIZE, 16) + "]"
           " + (ucs4 & 0x" + QByteArray::number(SMP_BLOCKSIZE-1, 16) + ")]))\n\n"
           "#define GET_PROP_INDEX_UCS2(ucs2) \\\n"
           "(uc_property_trie[uc_property_trie[ucs2>>" + QByteArray::number(BMP_SHIFT) +
           "] + (ucs2 & 0x" + QByteArray::number(BMP_BLOCKSIZE-1, 16)+ ")])\n\n"


           "static const QUnicodeTables::Properties uc_properties[] = {\n";

    // keep in sync with the property declaration
    for (int i = 0; i < uniqueProperties.size(); ++i) {
        PropertyFlags p = uniqueProperties.at(i);
        out += "    { ";
//     "        ushort category : 8;\n"
        out += QByteArray::number( p.category );
        out += ", ";
//     "        ushort line_break_class : 8;\n"
        out += QByteArray::number( p.line_break_class );
        out += ", ";
//     "        ushort direction : 8;\n"
        out += QByteArray::number( p.direction );
        out += ", ";
//     "        ushort combiningClass :8;\n"
        out += QByteArray::number( p.combiningClass );
        out += ", ";
//     "        ushort joining : 2;\n"
        out += QByteArray::number( p.joining );
        out += ", ";
//     "        signed short digitValue : 6;\n /* 5 needed */"
        out += QByteArray::number( p.digitValue );
        out += ", ";
//     "        ushort unicodeVersion : 4;\n"
        out += QByteArray::number( p.age );
        out += ", ";
//     "        ushort lowerCaseSpecial : 1;\n"
//     "        ushort upperCaseSpecial : 1;\n"
//     "        ushort titleCaseSpecial : 1;\n"
//     "        ushort caseFoldSpecial : 1;\n"
        out += QByteArray::number( p.lowerCaseSpecial );
        out += ", ";
        out += QByteArray::number( p.upperCaseSpecial );
        out += ", ";
        out += QByteArray::number( p.titleCaseSpecial );
        out += ", ";
        out += QByteArray::number( p.caseFoldSpecial );
        out += ", ";
//     "        signed short mirrorDiff : 16;\n"
//     "        signed short lowerCaseDiff : 16;\n"
//     "        signed short upperCaseDiff : 16;\n"
//     "        signed short titleCaseDiff : 16;\n"
//     "        signed short caseFoldDiff : 16;\n"
        out += QByteArray::number( p.mirrorDiff );
        out += ", ";
        out += QByteArray::number( p.lowerCaseDiff );
        out += ", ";
        out += QByteArray::number( p.upperCaseDiff );
        out += ", ";
        out += QByteArray::number( p.titleCaseDiff );
        out += ", ";
        out += QByteArray::number( p.caseFoldDiff );
        out += ", ";
        out += QByteArray::number( p.graphemeBreak );
        out += ", ";
        out += QByteArray::number( p.wordBreak );
        out += ", ";
        out += QByteArray::number( p.sentenceBreak );
        out += " },\n";
    }
    out += "};\n\n";

    out += "static inline const QUnicodeTables::Properties *qGetProp(uint ucs4)\n"
           "{\n"
           "    int index = GET_PROP_INDEX(ucs4);\n"
           "    return uc_properties + index;\n"
           "}\n"
           "\n"
           "static inline const QUnicodeTables::Properties *qGetProp(ushort ucs2)\n"
           "{\n"
           "    int index = GET_PROP_INDEX_UCS2(ucs2);\n"
           "    return uc_properties + index;\n"
           "}\n"
           "\n"
           "Q_CORE_EXPORT const QUnicodeTables::Properties * QT_FASTCALL QUnicodeTables::properties(uint ucs4)\n"
           "{\n"
           "    int index = GET_PROP_INDEX(ucs4);\n"
           "    return uc_properties + index;\n"
           "}\n"
           "\n"
           "Q_CORE_EXPORT const QUnicodeTables::Properties * QT_FASTCALL QUnicodeTables::properties(ushort ucs2)\n"
           "{\n"
           "    int index = GET_PROP_INDEX_UCS2(ucs2);\n"
           "    return uc_properties + index;\n"
           "}\n\n";

    out += "Q_CORE_EXPORT QUnicodeTables::LineBreakClass QT_FASTCALL QUnicodeTables::lineBreakClass(uint ucs4)\n"
           "{\n"
           "    return (QUnicodeTables::LineBreakClass)qGetProp(ucs4)->line_break_class;\n"
           "}\n\n";

    out += "static const ushort specialCaseMap[] = {\n   ";
    for (int i = 0; i < specialCaseMap.size(); ++i) {
        out += QByteArray(" 0x") + QByteArray::number(specialCaseMap.at(i), 16);
        if (i < specialCaseMap.size() - 1)
            out += ",";
        if (!specialCaseMap.at(i))
            out += "\n   ";
    }
    out += "\n};\n";
    out += "#define SPECIAL_CASE_MAX_LEN " + QByteArray::number(specialCaseMaxLen) + "\n\n";

    qDebug("Special case map uses : %d bytes", specialCaseMap.size()*2);

    return out;
}


struct DecompositionBlock {
    DecompositionBlock() { index = -1; }
    int index;
    QList<int> decompositionPositions;
    bool operator ==(const DecompositionBlock &other)
    { return decompositionPositions == other.decompositionPositions; }
};

static QByteArray createCompositionInfo()
{
    qDebug("createCompositionInfo:");

    const int BMP_BLOCKSIZE = 16;
    const int BMP_SHIFT = 4;
    const int BMP_END = 0x3400; // start of Han
    const int SMP_END = 0x30000;
    const int SMP_BLOCKSIZE = 256;
    const int SMP_SHIFT = 8;

    if (SMP_END <= highestComposedCharacter)
        qFatal("end of table smaller than highest composed character at %x", highestComposedCharacter);

    QList<DecompositionBlock> blocks;
    QList<int> blockMap;
    QList<unsigned short> decompositions;

    int used = 0;
    int tableIndex = 0;

    for (int block = 0; block < BMP_END/BMP_BLOCKSIZE; ++block) {
        DecompositionBlock b;
        for (int i = 0; i < BMP_BLOCKSIZE; ++i) {
            int uc = block*BMP_BLOCKSIZE + i;
            UnicodeData d = unicodeData.value(uc, UnicodeData(uc));
            if (!d.decomposition.isEmpty()) {
                int utf16Chars = 0;
                for (int j = 0; j < d.decomposition.size(); ++j)
                    utf16Chars += d.decomposition.at(j) >= 0x10000 ? 2 : 1;
                decompositions.append(d.decompositionType + (utf16Chars<<8));
                for (int j = 0; j < d.decomposition.size(); ++j) {
                    int code = d.decomposition.at(j);
                    if (code >= 0x10000) {
                        // save as surrogate pair
                        ushort high = QChar::highSurrogate(code);
                        ushort low = QChar::lowSurrogate(code);
                        decompositions.append(high);
                        decompositions.append(low);
                    } else {
                        decompositions.append(code);
                    }
                }
                b.decompositionPositions.append(tableIndex);
                tableIndex += utf16Chars + 1;
            } else {
                b.decompositionPositions.append(0xffff);
            }
        }
        int index = blocks.indexOf(b);
        if (index == -1) {
            index = blocks.size();
            b.index = used;
            used += BMP_BLOCKSIZE;
            blocks.append(b);
        }
        blockMap.append(blocks.at(index).index);
    }

    int bmp_blocks = blocks.size();
    Q_ASSERT(blockMap.size() == BMP_END/BMP_BLOCKSIZE);

    for (int block = BMP_END/SMP_BLOCKSIZE; block < SMP_END/SMP_BLOCKSIZE; ++block) {
        DecompositionBlock b;
        for (int i = 0; i < SMP_BLOCKSIZE; ++i) {
            int uc = block*SMP_BLOCKSIZE + i;
            UnicodeData d = unicodeData.value(uc, UnicodeData(uc));
            if (!d.decomposition.isEmpty()) {
                int utf16Chars = 0;
                for (int j = 0; j < d.decomposition.size(); ++j)
                    utf16Chars += d.decomposition.at(j) >= 0x10000 ? 2 : 1;
                decompositions.append(d.decompositionType + (utf16Chars<<8));
                for (int j = 0; j < d.decomposition.size(); ++j) {
                    int code = d.decomposition.at(j);
                    if (code >= 0x10000) {
                        // save as surrogate pair
                        ushort high = QChar::highSurrogate(code);
                        ushort low = QChar::lowSurrogate(code);
                        decompositions.append(high);
                        decompositions.append(low);
                    } else {
                        decompositions.append(code);
                    }
                }
                b.decompositionPositions.append(tableIndex);
                tableIndex += utf16Chars + 1;
            } else {
                b.decompositionPositions.append(0xffff);
            }
        }
        int index = blocks.indexOf(b);
        if (index == -1) {
            index = blocks.size();
            b.index = used;
            used += SMP_BLOCKSIZE;
            blocks.append(b);
        }
        blockMap.append(blocks.at(index).index);
    }

    int bmp_block_data = bmp_blocks*BMP_BLOCKSIZE*2;
    int bmp_trie = BMP_END/BMP_BLOCKSIZE*2;
    int bmp_mem = bmp_block_data + bmp_trie;
    qDebug("    %d unique blocks in BMP.", blocks.size());
    qDebug("        block data uses: %d bytes", bmp_block_data);
    qDebug("        trie data uses : %d bytes", bmp_trie);
    qDebug("        memory usage: %d bytes", bmp_mem);

    int smp_block_data = (blocks.size() - bmp_blocks)*SMP_BLOCKSIZE*2;
    int smp_trie = (SMP_END-BMP_END)/SMP_BLOCKSIZE*2;
    int smp_mem = smp_block_data + smp_trie;
    qDebug("    %d unique blocks in SMP.", blocks.size()-bmp_blocks);
    qDebug("        block data uses: %d bytes", smp_block_data);
    qDebug("        trie data uses : %d bytes", smp_trie);

    qDebug("\n        decomposition table use : %d bytes", decompositions.size()*2);
    qDebug("    memory usage: %d bytes", bmp_mem+smp_mem + decompositions.size()*2);

    QByteArray out;

    out += "static const unsigned short uc_decomposition_trie[] = {\n";

    // first write the map
    out += "    // 0 - 0x" + QByteArray::number(BMP_END, 16);
    for (int i = 0; i < BMP_END/BMP_BLOCKSIZE; ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!((i*BMP_BLOCKSIZE) % 0x1000))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n\n    // 0x" + QByteArray::number(BMP_END, 16) + " - 0x" + QByteArray::number(SMP_END, 16) + "\n";;
    for (int i = BMP_END/BMP_BLOCKSIZE; i < blockMap.size(); ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!(i % (0x10000/SMP_BLOCKSIZE)))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n";
    // write the data
    for (int i = 0; i < blocks.size(); ++i) {
        if (out.endsWith(' '))
            out.chop(1);
        out += "\n";
        const DecompositionBlock &b = blocks.at(i);
        for (int j = 0; j < b.decompositionPositions.size(); ++j) {
            if (!(j % 8)) {
                if (out.endsWith(' '))
                    out.chop(1);
                out += "\n    ";
            }
            out += "0x" + QByteArray::number(b.decompositionPositions.at(j), 16);
            out += ", ";
        }
    }

    if (out.endsWith(' '))
        out.chop(1);
    out += "\n};\n\n"

           "#define GET_DECOMPOSITION_INDEX(ucs4) \\\n"
           "       (ucs4 < 0x" + QByteArray::number(BMP_END, 16) + " \\\n"
           "        ? (uc_decomposition_trie[uc_decomposition_trie[ucs4>>" + QByteArray::number(BMP_SHIFT) +
           "] + (ucs4 & 0x" + QByteArray::number(BMP_BLOCKSIZE-1, 16)+ ")]) \\\n"
           "        : (ucs4 < 0x" + QByteArray::number(SMP_END, 16) + "\\\n"
           "           ? uc_decomposition_trie[uc_decomposition_trie[((ucs4 - 0x" + QByteArray::number(BMP_END, 16) +
           ")>>" + QByteArray::number(SMP_SHIFT) + ") + 0x" + QByteArray::number(BMP_END/BMP_BLOCKSIZE, 16) + "]"
           " + (ucs4 & 0x" + QByteArray::number(SMP_BLOCKSIZE-1, 16) + ")]\\\n"
           "           : 0xffff))\n\n"

           "static const unsigned short uc_decomposition_map[] = {\n";

    for (int i = 0; i < decompositions.size(); ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            out += "\n    ";
        }
        out += "0x" + QByteArray::number(decompositions.at(i), 16);
        out += ", ";
    }

    if (out.endsWith(' '))
        out.chop(1);
    out += "\n};\n\n";

    return out;
}

static QByteArray createLigatureInfo()
{
    qDebug("createLigatureInfo: numLigatures=%d", numLigatures);

    QList<DecompositionBlock> blocks;
    QList<int> blockMap;
    QList<unsigned short> ligatures;

    const int BMP_BLOCKSIZE = 32;
    const int BMP_SHIFT = 5;
    const int BMP_END = 0x3100;
    Q_ASSERT(highestLigature < BMP_END);

    int used = 0;
    int tableIndex = 0;

    for (int block = 0; block < BMP_END/BMP_BLOCKSIZE; ++block) {
        DecompositionBlock b;
        for (int i = 0; i < BMP_BLOCKSIZE; ++i) {
            int uc = block*BMP_BLOCKSIZE + i;
            QList<Ligature> l = ligatureHashes.value(uc);
            if (!l.isEmpty()) {
                qSort(l);

                ligatures.append(l.size());
                for (int j = 0; j < l.size(); ++j) {
                    Q_ASSERT(l.at(j).u2 == uc);
                    ligatures.append(l.at(j).u1);
                    ligatures.append(l.at(j).ligature);
                }
                b.decompositionPositions.append(tableIndex);
                tableIndex += 2*l.size() + 1;
            } else {
                b.decompositionPositions.append(0xffff);
            }
        }
        int index = blocks.indexOf(b);
        if (index == -1) {
            index = blocks.size();
            b.index = used;
            used += BMP_BLOCKSIZE;
            blocks.append(b);
        }
        blockMap.append(blocks.at(index).index);
    }

    int bmp_blocks = blocks.size();
    Q_ASSERT(blockMap.size() == BMP_END/BMP_BLOCKSIZE);

    int bmp_block_data = bmp_blocks*BMP_BLOCKSIZE*2;
    int bmp_trie = BMP_END/BMP_BLOCKSIZE*2;
    int bmp_mem = bmp_block_data + bmp_trie;
    qDebug("    %d unique blocks in BMP.", blocks.size());
    qDebug("        block data uses: %d bytes", bmp_block_data);
    qDebug("        trie data uses : %d bytes", bmp_trie);
    qDebug("\n        ligature data uses : %d bytes", ligatures.size()*2);
    qDebug("    memory usage: %d bytes", bmp_mem + ligatures.size() * 2);

    QByteArray out;

    out += "static const unsigned short uc_ligature_trie[] = {\n";

    // first write the map
    out += "    // 0 - 0x" + QByteArray::number(BMP_END, 16);
    for (int i = 0; i < BMP_END/BMP_BLOCKSIZE; ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            if (!((i*BMP_BLOCKSIZE) % 0x1000))
                out += "\n";
            out += "\n    ";
        }
        out += QByteArray::number(blockMap.at(i) + blockMap.size());
        out += ", ";
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n";
    // write the data
    for (int i = 0; i < blocks.size(); ++i) {
        if (out.endsWith(' '))
            out.chop(1);
        out += "\n";
        const DecompositionBlock &b = blocks.at(i);
        for (int j = 0; j < b.decompositionPositions.size(); ++j) {
            if (!(j % 8)) {
                if (out.endsWith(' '))
                    out.chop(1);
                out += "\n    ";
            }
            out += "0x" + QByteArray::number(b.decompositionPositions.at(j), 16);
            out += ", ";
        }
    }
    if (out.endsWith(' '))
        out.chop(1);
    out += "\n};\n\n"

           "#define GET_LIGATURE_INDEX(u2) "
           "(u2 < 0x" + QByteArray::number(BMP_END, 16) + " ? "
           "uc_ligature_trie[uc_ligature_trie[u2>>" + QByteArray::number(BMP_SHIFT) +
           "] + (u2 & 0x" + QByteArray::number(BMP_BLOCKSIZE-1, 16)+ ")] : 0xffff);\n\n"

           "static const unsigned short uc_ligature_map[] = {\n";

    for (int i = 0; i < ligatures.size(); ++i) {
        if (!(i % 8)) {
            if (out.endsWith(' '))
                out.chop(1);
            out += "\n    ";
        }
        out += "0x" + QByteArray::number(ligatures.at(i), 16);
        out += ", ";
    }

    if (out.endsWith(' '))
        out.chop(1);
    out += "\n};\n\n";

    return out;
}

QByteArray createCasingInfo()
{
    QByteArray out;

    out += "struct CasingInfo {\n"
           "    uint codePoint : 16;\n"
           "    uint flags : 8;\n"
           "    uint offset : 8;\n"
           "};\n\n";

    return out;
}


int main(int, char **)
{
    initAgeMap();
    initCategoryMap();
    initDecompositionMap();
    initDirectionMap();
    initJoiningMap();
    initGraphemeBreak();
    initWordBreak();
    initSentenceBreak();
    initLineBreak();

    readUnicodeData();
    readBidiMirroring();
    readArabicShaping();
    readDerivedAge();
    readDerivedNormalizationProps();
    readSpecialCasing();
    readCaseFolding();
    // readBlocks();
    readScripts();
    readGraphemeBreak();
    readWordBreak();
    readSentenceBreak();
    readLineBreak();

    computeUniqueProperties();
    QByteArray properties = createPropertyInfo();
    QByteArray compositions = createCompositionInfo();
    QByteArray ligatures = createLigatureInfo();
    QByteArray normalizationCorrections = createNormalizationCorrections();
    QByteArray scriptEnumDeclaration = createScriptEnumDeclaration();
    QByteArray scriptTableDeclaration = createScriptTableDeclaration();

    QByteArray header =
        "/****************************************************************************\n"
        "**\n"
        "** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).\n"
        "** All rights reserved.\n"
        "** Contact: Nokia Corporation (qt-info@nokia.com)\n"
        "**\n"
        "** This file is part of the QtCore module of the Qt Toolkit.\n"
        "**\n"
        "** $QT_BEGIN_LICENSE:LGPL$\n"
        "** GNU Lesser General Public License Usage\n"
        "** This file may be used under the terms of the GNU Lesser General Public\n"
        "** License version 2.1 as published by the Free Software Foundation and\n"
        "** appearing in the file LICENSE.LGPL included in the packaging of this\n"
        "** file. Please review the following information to ensure the GNU Lesser\n"
        "** General Public License version 2.1 requirements will be met:\n"
        "** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.\n"
        "**\n"
        "** In addition, as a special exception, Nokia gives you certain additional\n"
        "** rights. These rights are described in the Nokia Qt LGPL Exception\n"
        "** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.\n"
        "**\n"
        "** GNU General Public License Usage\n"
        "** Alternatively, this file may be used under the terms of the GNU General\n"
        "** Public License version 3.0 as published by the Free Software Foundation\n"
        "** and appearing in the file LICENSE.GPL included in the packaging of this\n"
        "** file. Please review the following information to ensure the GNU General\n"
        "** Public License version 3.0 requirements will be met:\n"
        "** http://www.gnu.org/copyleft/gpl.html.\n"
        "**\n"
        "** Other Usage\n"
        "** Alternatively, this file may be used in accordance with the terms and\n"
        "** conditions contained in a signed written agreement between you and Nokia.\n"
        "**\n"
        "**\n"
        "**\n"
        "**\n"
        "**\n"
        "** $QT_END_LICENSE$\n"
        "**\n"
        "****************************************************************************/\n\n";

    QByteArray note =
        "/* This file is autogenerated from the Unicode "DATA_VERSION_S" database. Do not edit */\n\n";

    QByteArray warning =
        "//\n"
        "//  W A R N I N G\n"
        "//  -------------\n"
        "//\n"
        "// This file is not part of the Qt API.  It exists for the convenience\n"
        "// of internal files.  This header file may change from version to version\n"
        "// without notice, or even be removed.\n"
        "//\n"
        "// We mean it.\n"
        "//\n\n";

    QFile f("../../src/corelib/tools/qunicodetables.cpp");
    f.open(QFile::WriteOnly|QFile::Truncate);
    f.write(header);
    f.write(note);
    f.write("QT_BEGIN_NAMESPACE\n\n");
    f.write(properties);
    f.write(compositions);
    f.write(ligatures);
    f.write(normalizationCorrections);
    f.write(scriptTableDeclaration);
    f.write("QT_END_NAMESPACE\n");
    f.close();

    f.setFileName("../../src/corelib/tools/qunicodetables_p.h");
    f.open(QFile::WriteOnly | QFile::Truncate);
    f.write(header);
    f.write(note);
    f.write(warning);
    f.write("#ifndef QUNICODETABLES_P_H\n"
            "#define QUNICODETABLES_P_H\n\n"
            "#include <QtCore/qchar.h>\n\n"
            "QT_BEGIN_NAMESPACE\n\n");
    f.write("#define UNICODE_DATA_VERSION "DATA_VERSION_STR"\n\n");
    f.write("#define UNICODE_LAST_CODEPOINT "LAST_CODEPOINT_STR"\n\n");
    f.write("namespace QUnicodeTables {\n\n");
    f.write(property_string);
    f.write("\n");
    f.write(scriptEnumDeclaration);
    f.write("\n");
    f.write(grapheme_break_string);
    f.write("\n");
    f.write(word_break_string);
    f.write("\n");
    f.write(sentence_break_string);
    f.write("\n");
    f.write(lineBreakClass);
    f.write("\n");
    f.write(methods);
    f.write("} // namespace QUnicodeTables\n\n"
            "QT_END_NAMESPACE\n\n"
            "#endif // QUNICODETABLES_P_H\n");
    f.close();

    qDebug() << "maxMirroredDiff  = " << hex << maxMirroredDiff;
    qDebug() << "maxLowerCaseDiff = " << hex << maxLowerCaseDiff;
    qDebug() << "maxUpperCaseDiff = " << hex << maxUpperCaseDiff;
    qDebug() << "maxTitleCaseDiff = " << hex << maxTitleCaseDiff;
    qDebug() << "maxCaseFoldDiff  = " << hex << maxCaseFoldDiff;
#if 0
//     dump(0, 0x7f);
//     dump(0x620, 0x640);
//     dump(0x10000, 0x10020);
//     dump(0x10800, 0x10820);

    qDebug("decompositionLength used:");
    int totalcompositions = 0;
    int sum = 0;
    for (int i = 1; i < 20; ++i) {
        qDebug("    length %d used %d times", i, decompositionLength.value(i, 0));
        totalcompositions += i*decompositionLength.value(i, 0);
        sum += decompositionLength.value(i, 0);
    }
    qDebug("    len decomposition map %d, average length %f, num composed chars %d",
           totalcompositions, (float)totalcompositions/(float)sum, sum);
    qDebug("highest composed character %x", highestComposedCharacter);
    qDebug("num ligatures = %d highest=%x, maxLength=%d", numLigatures, highestLigature, longestLigature);

    qBubbleSort(ligatures);
    for (int i = 0; i < ligatures.size(); ++i)
        qDebug("%s", ligatures.at(i).data());

//     qDebug("combiningClass usage:");
//     int numClasses = 0;
//     for (int i = 0; i < 255; ++i) {
//         int num = combiningClassUsage.value(i, 0);
//         if (num) {
//             ++numClasses;
//             qDebug("    combiningClass %d used %d times", i, num);
//         }
//     }
//     qDebug("total of %d combining classes used", numClasses);

#endif
}
