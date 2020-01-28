/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL-EXCEPT$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 as published by the Free Software
** Foundation with exceptions as appearing in the file LICENSE.GPL3-EXCEPT
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "textdump.h"

#include <QTextStream>
#include <QString>

namespace QtDiag {

struct EnumLookup {
    int value;
    const char *description;
};

static const EnumLookup specialCharactersEnumLookup[] =
{
    {QChar::Null, "Null"},
#if QT_VERSION >= 0x050000
    {QChar::Tabulation, "Tabulation"},
    {QChar::LineFeed, "LineFeed"},
#  if QT_VERSION >= 0x050e00
    {QChar::FormFeed, "FormFeed"},
#  endif
    {QChar::CarriageReturn, "CarriageReturn"},
    {QChar::Space, "Space"},
#endif
    {QChar::Nbsp, "Nbsp"},
#if QT_VERSION >= 0x050000
    {QChar::SoftHyphen, "SoftHyphen"},
#endif
    {QChar::ReplacementCharacter, "ReplacementCharacter"},
    {QChar::ObjectReplacementCharacter, "ObjectReplacementCharacter"},
    {QChar::ByteOrderMark, "ByteOrderMark"},
    {QChar::ByteOrderSwapped, "ByteOrderSwapped"},
    {QChar::ParagraphSeparator, "ParagraphSeparator"},
    {QChar::LineSeparator, "LineSeparator"},
#if QT_VERSION >= 0x050000
    {QChar::LastValidCodePoint, "LastValidCodePoint"}
#endif
};

static const EnumLookup categoryEnumLookup[] =
{
    {QChar::Mark_NonSpacing, "Mark_NonSpacing"},
    {QChar::Mark_SpacingCombining, "Mark_SpacingCombining"},
    {QChar::Mark_Enclosing, "Mark_Enclosing"},

    {QChar::Number_DecimalDigit, "Number_DecimalDigit"},
    {QChar::Number_Letter, "Number_Letter"},
    {QChar::Number_Other, "Number_Other"},

    {QChar::Separator_Space, "Separator_Space"},
    {QChar::Separator_Line, "Separator_Line"},
    {QChar::Separator_Paragraph, "Separator_Paragraph"},

    {QChar::Other_Control, "Other_Control"},
    {QChar::Other_Format, "Other_Format"},
    {QChar::Other_Surrogate, "Other_Surrogate"},
    {QChar::Other_PrivateUse, "Other_PrivateUse"},
    {QChar::Other_NotAssigned, "Other_NotAssigned"},

    {QChar::Letter_Uppercase, "Letter_Uppercase"},
    {QChar::Letter_Lowercase, "Letter_Lowercase"},
    {QChar::Letter_Titlecase, "Letter_Titlecase"},
    {QChar::Letter_Modifier, "Letter_Modifier"},
    {QChar::Letter_Other, "Letter_Other"},

    {QChar::Punctuation_Connector, "Punctuation_Connector"},
    {QChar::Punctuation_Dash, "Punctuation_Dash"},
    {QChar::Punctuation_Open, "Punctuation_Open"},
    {QChar::Punctuation_Close, "Punctuation_Close"},
    {QChar::Punctuation_InitialQuote, "Punctuation_InitialQuote"},
    {QChar::Punctuation_FinalQuote, "Punctuation_FinalQuote"},
    {QChar::Punctuation_Other, "Punctuation_Other"},

    {QChar::Symbol_Math, "Symbol_Math"},
    {QChar::Symbol_Currency, "Symbol_Currency"},
    {QChar::Symbol_Modifier, "Symbol_Modifier"},
    {QChar::Symbol_Other, "Symbol_Other"},
};

#if QT_VERSION >= 0x050100

static const EnumLookup scriptEnumLookup[] =
{
    {QChar::Script_Unknown, "Script_Unknown"},
    {QChar::Script_Inherited, "Script_Inherited"},
    {QChar::Script_Common, "Script_Common"},

    {QChar::Script_Latin, "Script_Latin"},
    {QChar::Script_Greek, "Script_Greek"},
    {QChar::Script_Cyrillic, "Script_Cyrillic"},
    {QChar::Script_Armenian, "Script_Armenian"},
    {QChar::Script_Hebrew, "Script_Hebrew"},
    {QChar::Script_Arabic, "Script_Arabic"},
    {QChar::Script_Syriac, "Script_Syriac"},
    {QChar::Script_Thaana, "Script_Thaana"},
    {QChar::Script_Devanagari, "Script_Devanagari"},
    {QChar::Script_Bengali, "Script_Bengali"},
    {QChar::Script_Gurmukhi, "Script_Gurmukhi"},
    {QChar::Script_Gujarati, "Script_Gujarati"},
    {QChar::Script_Oriya, "Script_Oriya"},
    {QChar::Script_Tamil, "Script_Tamil"},
    {QChar::Script_Telugu, "Script_Telugu"},
    {QChar::Script_Kannada, "Script_Kannada"},
    {QChar::Script_Malayalam, "Script_Malayalam"},
    {QChar::Script_Sinhala, "Script_Sinhala"},
    {QChar::Script_Thai, "Script_Thai"},
    {QChar::Script_Lao, "Script_Lao"},
    {QChar::Script_Tibetan, "Script_Tibetan"},
    {QChar::Script_Myanmar, "Script_Myanmar"},
    {QChar::Script_Georgian, "Script_Georgian"},
    {QChar::Script_Hangul, "Script_Hangul"},
    {QChar::Script_Ethiopic, "Script_Ethiopic"},
    {QChar::Script_Cherokee, "Script_Cherokee"},
    {QChar::Script_CanadianAboriginal, "Script_CanadianAboriginal"},
    {QChar::Script_Ogham, "Script_Ogham"},
    {QChar::Script_Runic, "Script_Runic"},
    {QChar::Script_Khmer, "Script_Khmer"},
    {QChar::Script_Mongolian, "Script_Mongolian"},
    {QChar::Script_Hiragana, "Script_Hiragana"},
    {QChar::Script_Katakana, "Script_Katakana"},
    {QChar::Script_Bopomofo, "Script_Bopomofo"},
    {QChar::Script_Han, "Script_Han"},
    {QChar::Script_Yi, "Script_Yi"},
    {QChar::Script_OldItalic, "Script_OldItalic"},
    {QChar::Script_Gothic, "Script_Gothic"},
    {QChar::Script_Deseret, "Script_Deseret"},
    {QChar::Script_Tagalog, "Script_Tagalog"},
    {QChar::Script_Hanunoo, "Script_Hanunoo"},
    {QChar::Script_Buhid, "Script_Buhid"},
    {QChar::Script_Tagbanwa, "Script_Tagbanwa"},
    {QChar::Script_Coptic, "Script_Coptic"},

    {QChar::Script_Limbu, "Script_Limbu"},
    {QChar::Script_TaiLe, "Script_TaiLe"},
    {QChar::Script_LinearB, "Script_LinearB"},
    {QChar::Script_Ugaritic, "Script_Ugaritic"},
    {QChar::Script_Shavian, "Script_Shavian"},
    {QChar::Script_Osmanya, "Script_Osmanya"},
    {QChar::Script_Cypriot, "Script_Cypriot"},
    {QChar::Script_Braille, "Script_Braille"},

    {QChar::Script_Buginese, "Script_Buginese"},
    {QChar::Script_NewTaiLue, "Script_NewTaiLue"},
    {QChar::Script_Glagolitic, "Script_Glagolitic"},
    {QChar::Script_Tifinagh, "Script_Tifinagh"},
    {QChar::Script_SylotiNagri, "Script_SylotiNagri"},
    {QChar::Script_OldPersian, "Script_OldPersian"},
    {QChar::Script_Kharoshthi, "Script_Kharoshthi"},

    {QChar::Script_Balinese, "Script_Balinese"},
    {QChar::Script_Cuneiform, "Script_Cuneiform"},
    {QChar::Script_Phoenician, "Script_Phoenician"},
    {QChar::Script_PhagsPa, "Script_PhagsPa"},
    {QChar::Script_Nko, "Script_Nko"},

    {QChar::Script_Sundanese, "Script_Sundanese"},
    {QChar::Script_Lepcha, "Script_Lepcha"},
    {QChar::Script_OlChiki, "Script_OlChiki"},
    {QChar::Script_Vai, "Script_Vai"},
    {QChar::Script_Saurashtra, "Script_Saurashtra"},
    {QChar::Script_KayahLi, "Script_KayahLi"},
    {QChar::Script_Rejang, "Script_Rejang"},
    {QChar::Script_Lycian, "Script_Lycian"},
    {QChar::Script_Carian, "Script_Carian"},
    {QChar::Script_Lydian, "Script_Lydian"},
    {QChar::Script_Cham, "Script_Cham"},

    {QChar::Script_TaiTham, "Script_TaiTham"},
    {QChar::Script_TaiViet, "Script_TaiViet"},
    {QChar::Script_Avestan, "Script_Avestan"},
    {QChar::Script_EgyptianHieroglyphs, "Script_EgyptianHieroglyphs"},
    {QChar::Script_Samaritan, "Script_Samaritan"},
    {QChar::Script_Lisu, "Script_Lisu"},
    {QChar::Script_Bamum, "Script_Bamum"},
    {QChar::Script_Javanese, "Script_Javanese"},
    {QChar::Script_MeeteiMayek, "Script_MeeteiMayek"},
    {QChar::Script_ImperialAramaic, "Script_ImperialAramaic"},
    {QChar::Script_OldSouthArabian, "Script_OldSouthArabian"},
    {QChar::Script_InscriptionalParthian, "Script_InscriptionalParthian"},
    {QChar::Script_InscriptionalPahlavi, "Script_InscriptionalPahlavi"},
    {QChar::Script_OldTurkic, "Script_OldTurkic"},
    {QChar::Script_Kaithi, "Script_Kaithi"},

    {QChar::Script_Batak, "Script_Batak"},
    {QChar::Script_Brahmi, "Script_Brahmi"},
    {QChar::Script_Mandaic, "Script_Mandaic"},

    {QChar::Script_Chakma, "Script_Chakma"},
    {QChar::Script_MeroiticCursive, "Script_MeroiticCursive"},
    {QChar::Script_MeroiticHieroglyphs, "Script_MeroiticHieroglyphs"},
    {QChar::Script_Miao, "Script_Miao"},
    {QChar::Script_Sharada, "Script_Sharada"},
    {QChar::Script_SoraSompeng, "Script_SoraSompeng"},
    {QChar::Script_Takri, "Script_Takri"},

#if QT_VERSION >= 0x050500
    {QChar::Script_CaucasianAlbanian, "Script_CaucasianAlbanian"},
    {QChar::Script_BassaVah, "Script_BassaVah"},
    {QChar::Script_Duployan, "Script_Duployan"},
    {QChar::Script_Elbasan, "Script_Elbasan"},
    {QChar::Script_Grantha, "Script_Grantha"},
    {QChar::Script_PahawhHmong, "Script_PahawhHmong"},
    {QChar::Script_Khojki, "Script_Khojki"},
    {QChar::Script_LinearA, "Script_LinearA"},
    {QChar::Script_Mahajani, "Script_Mahajani"},
    {QChar::Script_Manichaean, "Script_Manichaean"},
    {QChar::Script_MendeKikakui, "Script_MendeKikakui"},
    {QChar::Script_Modi, "Script_Modi"},
    {QChar::Script_Mro, "Script_Mro"},
    {QChar::Script_OldNorthArabian, "Script_OldNorthArabian"},
    {QChar::Script_Nabataean, "Script_Nabataean"},
    {QChar::Script_Palmyrene, "Script_Palmyrene"},
    {QChar::Script_PauCinHau, "Script_PauCinHau"},
    {QChar::Script_OldPermic, "Script_OldPermic"},
    {QChar::Script_PsalterPahlavi, "Script_PsalterPahlavi"},
    {QChar::Script_Siddham, "Script_Siddham"},
    {QChar::Script_Khudawadi, "Script_Khudawadi"},
    {QChar::Script_Tirhuta, "Script_Tirhuta"},
    {QChar::Script_WarangCiti, "Script_WarangCiti"},
#endif // Qt 5.5

#if QT_VERSION >= 0x050600
    {QChar::Script_Ahom, "Script_Ahom"},
    {QChar::Script_AnatolianHieroglyphs, "Script_AnatolianHieroglyphs"},
    {QChar::Script_Hatran, "Script_Hatran"},
    {QChar::Script_Multani, "Script_Multani"},
    {QChar::Script_OldHungarian, "Script_OldHungarian"},
    {QChar::Script_SignWriting, "Script_SignWriting"},
#endif // Qt 5.5
};

#endif // Qt 5.1

static const EnumLookup directionEnumLookup[] =
{
    {QChar::DirL, "DirL"},
    {QChar::DirR, "DirR"},
    {QChar::DirEN, "DirEN"},
    {QChar::DirES, "DirES"},
    {QChar::DirET, "DirET"},
    {QChar::DirAN, "DirAN"},
    {QChar::DirCS, "DirCS"},
    {QChar::DirB, "DirB"},
    {QChar::DirS, "DirS"},
    {QChar::DirWS, "DirWS"},
    {QChar::DirON, "DirON"},
    {QChar::DirLRE, "DirLRE"},
    {QChar::DirLRO, "DirLRO"},
    {QChar::DirAL, "DirAL"},
    {QChar::DirRLE, "DirRLE"},
    {QChar::DirRLO, "DirRLO"},
    {QChar::DirPDF, "DirPDF"},
    {QChar::DirNSM, "DirNSM"},
    {QChar::DirBN, "DirBN"},
#if QT_VERSION >= 0x050000
    {QChar::DirLRI, "DirLRI"},
    {QChar::DirRLI, "DirRLI"},
    {QChar::DirFSI, "DirFSI"},
    {QChar::DirPDI, "DirPDI"},
#endif
};

static const EnumLookup decompositionEnumLookup[] =
{
    {QChar::NoDecomposition, "NoDecomposition"},
    {QChar::Canonical, "Canonical"},
    {QChar::Font, "Font"},
    {QChar::NoBreak, "NoBreak"},
    {QChar::Initial, "Initial"},
    {QChar::Medial, "Medial"},
    {QChar::Final, "Final"},
    {QChar::Isolated, "Isolated"},
    {QChar::Circle, "Circle"},
    {QChar::Super, "Super"},
    {QChar::Sub, "Sub"},
    {QChar::Vertical, "Vertical"},
    {QChar::Wide, "Wide"},
    {QChar::Narrow, "Narrow"},
    {QChar::Small, "Small"},
    {QChar::Square, "Square"},
    {QChar::Compat, "Compat"},
    {QChar::Fraction, "Fraction"},
};

#if QT_VERSION >= 0x050000

static const EnumLookup joiningTypeEnumLookup[] =
{
    {QChar::Joining_None, "Joining_None"},
    {QChar::Joining_Causing, "Joining_Causing"},
    {QChar::Joining_Dual, "Joining_Dual"},
    {QChar::Joining_Right, "Joining_Right"},
    {QChar::Joining_Left, "Joining_Left"},
    {QChar::Joining_Transparent, "Joining_Transparent"}
};

#endif // Qt 5

static const EnumLookup combiningClassEnumLookup[] =
{
    {QChar::Combining_BelowLeftAttached, "Combining_BelowLeftAttached"},
    {QChar::Combining_BelowAttached, "Combining_BelowAttached"},
    {QChar::Combining_BelowRightAttached, "Combining_BelowRightAttached"},
    {QChar::Combining_LeftAttached, "Combining_LeftAttached"},
    {QChar::Combining_RightAttached, "Combining_RightAttached"},
    {QChar::Combining_AboveLeftAttached, "Combining_AboveLeftAttached"},
    {QChar::Combining_AboveAttached, "Combining_AboveAttached"},
    {QChar::Combining_AboveRightAttached, "Combining_AboveRightAttached"},

    {QChar::Combining_BelowLeft, "Combining_BelowLeft"},
    {QChar::Combining_Below, "Combining_Below"},
    {QChar::Combining_BelowRight, "Combining_BelowRight"},
    {QChar::Combining_Left, "Combining_Left"},
    {QChar::Combining_Right, "Combining_Right"},
    {QChar::Combining_AboveLeft, "Combining_AboveLeft"},
    {QChar::Combining_Above, "Combining_Above"},
    {QChar::Combining_AboveRight, "Combining_AboveRight"},

    {QChar::Combining_DoubleBelow, "Combining_DoubleBelow"},
    {QChar::Combining_DoubleAbove, "Combining_DoubleAbove"},
    {QChar::Combining_IotaSubscript, "Combining_IotaSubscript"},
};

static const EnumLookup unicodeVersionEnumLookup[] =
{
    {QChar::Unicode_Unassigned, "Unicode_Unassigned"},
    {QChar::Unicode_1_1, "Unicode_1_1"},
    {QChar::Unicode_2_0, "Unicode_2_0"},
    {QChar::Unicode_2_1_2, "Unicode_2_1_2"},
    {QChar::Unicode_3_0, "Unicode_3_0"},
    {QChar::Unicode_3_1, "Unicode_3_1"},
    {QChar::Unicode_3_2, "Unicode_3_2"},
    {QChar::Unicode_4_0, "Unicode_4_0"},
    {QChar::Unicode_4_1, "Unicode_4_1"},
    {QChar::Unicode_5_0, "Unicode_5_0"},
#if QT_VERSION >= 0x050000
    {QChar::Unicode_5_1, "Unicode_5_1"},
    {QChar::Unicode_5_2, "Unicode_5_2"},
    {QChar::Unicode_6_0, "Unicode_6_0"},
    {QChar::Unicode_6_1, "Unicode_6_1"},
    {QChar::Unicode_6_2, "Unicode_6_2"},
    {QChar::Unicode_6_3, "Unicode_6_3"},
#if QT_VERSION >= 0x050500
    {QChar::Unicode_7_0, "Unicode_7_0"},
#endif // Qt 5.5
#if QT_VERSION >= 0x050600
    {QChar::Unicode_8_0, "Unicode_8_0"},
#endif // Qt 5.6
#endif // Qt 5
};

static const EnumLookup *enumLookup(int v, const EnumLookup *array, size_t size)
{
    const EnumLookup *end = array + size;
    for (const EnumLookup *p = array; p < end; ++p) {
        if (p->value == v)
            return p;
    }
    return nullptr;
}

static const char *enumName(int v, const EnumLookup *array, size_t size)
{
    const EnumLookup *e = enumLookup(v, array, size);
    return e ? e->description : "<unknown>";
}

// Context struct storing the parameters of the last character, only the parameters
// that change will be output.
struct FormattingContext
{
    int category = -1;
    int direction = -1;
    int joiningType = -1;
    int decompositionTag = -1;
    int script = -1;
    int unicodeVersion = -1;
};

static void formatCharacter(QTextStream &str, const QChar &qc, FormattingContext &context)
{
    const ushort unicode = qc.unicode();
    str << "U+" << qSetFieldWidth(4) << qSetPadChar('0') << Qt::uppercasedigits
        << Qt::hex << unicode << Qt::dec << qSetFieldWidth(0) << ' ';

    const EnumLookup *specialChar = enumLookup(unicode, specialCharactersEnumLookup, sizeof(specialCharactersEnumLookup) / sizeof(EnumLookup));
    if (specialChar)
        str << specialChar->description;
    else
        str << "'" << qc << '\'';

    const int category = qc.category();
    if (category != context.category) {
        str << " category="
            << enumName(category, categoryEnumLookup, sizeof(categoryEnumLookup) / sizeof(EnumLookup));
        context.category = category;
    }
#if QT_VERSION >= 0x050100
    const int script = qc.script();
    if (script != context.script) {
        str << " script="
            << enumName(script, scriptEnumLookup, sizeof(scriptEnumLookup) / sizeof(EnumLookup))
            << '(' << script << ')';
        context.script = script;
    }
#endif // Qt 5
    const int direction = qc.direction();
    if (direction != context.direction) {
        str << " direction="
            << enumName(direction, directionEnumLookup, sizeof(directionEnumLookup) / sizeof(EnumLookup));
        context.direction = direction;
    }
#if QT_VERSION >= 0x050000
    const int joiningType = qc.joiningType();
    if (joiningType != context.joiningType) {
        str << " joiningType="
            << enumName(joiningType, joiningTypeEnumLookup, sizeof(joiningTypeEnumLookup) / sizeof(EnumLookup));
        context.joiningType = joiningType;
    }
#endif // Qt 5QWidget
    const int decompositionTag = qc.decompositionTag();
    if (decompositionTag != context.decompositionTag) {
        str << " decomposition="
            << enumName(decompositionTag, decompositionEnumLookup, sizeof(decompositionEnumLookup) / sizeof(EnumLookup));
        context.decompositionTag = decompositionTag;
    }
    const int unicodeVersion = qc.unicodeVersion();
    if (unicodeVersion != context.unicodeVersion) {
        str << " version="
        << enumName(unicodeVersion, unicodeVersionEnumLookup, sizeof(unicodeVersionEnumLookup) / sizeof(EnumLookup));
        context.unicodeVersion = unicodeVersion;
    }
}

QString dumpText(const QString &text)
{
    QString result;
    QTextStream str(&result);
    FormattingContext context;
    for (int i = 0; i < text.size(); ++i) {
        str << '#' << (i + 1) << ' ';
        formatCharacter(str, text.at(i), context);
        str << '\n';
    }
    return result;
}

QString dumpTextAsCode(const QString &text)
{
    QString result;
    QTextStream str(&result);
    str << "    QString result;\n" << Qt::hex << Qt::showbase;
    for (QChar c : text)
        str << "    result += QChar(" << c.unicode() << ");\n";
    str << '\n';
    return result;
}

} // namespace QtDiag
