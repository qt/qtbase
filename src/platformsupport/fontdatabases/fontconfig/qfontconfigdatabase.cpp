/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qfontconfigdatabase_p.h"
#include "qfontenginemultifontconfig_p.h"

#include <QtCore/QList>
#include <QtGui/private/qfont_p.h>

#include <QtCore/QElapsedTimer>

#include <qpa/qplatformnativeinterface.h>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformintegration.h>
#include <qpa/qplatformservices.h>

#include <QtGui/private/qfontengine_ft_p.h>
#include <QtGui/private/qfontengine_p.h>
#include <QtGui/private/qfontengine_qpa_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <QtGui/qguiapplication.h>

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H

#include <fontconfig/fontconfig.h>
#include FT_FREETYPE_H

#if FC_VERSION >= 20402
#include <fontconfig/fcfreetype.h>
#endif

#define SimplifiedChineseCsbBit 18
#define TraditionalChineseCsbBit 20
#define JapaneseCsbBit 17
#define KoreanCsbBit 21

QT_BEGIN_NAMESPACE

static inline bool requiresOpenType(int writingSystem)
{
    return ((writingSystem >= QFontDatabase::Syriac && writingSystem <= QFontDatabase::Sinhala)
            || writingSystem == QFontDatabase::Khmer || writingSystem == QFontDatabase::Nko);
}

static inline bool scriptRequiresOpenType(int script)
{
    return ((script >= QChar::Script_Syriac && script <= QChar::Script_Sinhala)
            || script == QChar::Script_Khmer || script == QChar::Script_Nko);
}

static int getFCWeight(int fc_weight)
{
    int qtweight = QFont::Black;
    if (fc_weight <= (FC_WEIGHT_LIGHT + FC_WEIGHT_REGULAR) / 2)
        qtweight = QFont::Light;
    else if (fc_weight <= (FC_WEIGHT_REGULAR + FC_WEIGHT_MEDIUM) / 2)
        qtweight = QFont::Normal;
    else if (fc_weight <= (FC_WEIGHT_MEDIUM + FC_WEIGHT_BOLD) / 2)
        qtweight = QFont::DemiBold;
    else if (fc_weight <= (FC_WEIGHT_BOLD + FC_WEIGHT_BLACK) / 2)
        qtweight = QFont::Bold;

    return qtweight;
}

static const char *specialLanguages[] = {
    "", // Unknown
    "", // Inherited
    "", // Common
    "en", // Latin
    "el", // Greek
    "ru", // Cyrillic
    "hy", // Armenian
    "he", // Hebrew
    "ar", // Arabic
    "syr", // Syriac
    "dv", // Thaana
    "hi", // Devanagari
    "bn", // Bengali
    "pa", // Gurmukhi
    "gu", // Gujarati
    "or", // Oriya
    "ta", // Tamil
    "te", // Telugu
    "kn", // Kannada
    "ml", // Malayalam
    "si", // Sinhala
    "th", // Thai
    "lo", // Lao
    "bo", // Tibetan
    "my", // Myanmar
    "ka", // Georgian
    "ko", // Hangul
    "am", // Ethiopic
    "chr", // Cherokee
    "cr", // CanadianAboriginal
    "sga", // Ogham
    "non", // Runic
    "km", // Khmer
    "mn", // Mongolian
    "ja", // Hiragana
    "ja", // Katakana
    "zh-TW", // Bopomofo
    "", // Han
    "ii", // Yi
    "ett", // OldItalic
    "got", // Gothic
    "en", // Deseret
    "fil", // Tagalog
    "hnn", // Hanunoo
    "bku", // Buhid
    "tbw", // Tagbanwa
    "cop", // Coptic
    "lif", // Limbu
    "tdd", // TaiLe
    "grc", // LinearB
    "uga", // Ugaritic
    "en", // Shavian
    "so", // Osmanya
    "grc", // Cypriot
    "", // Braille
    "bug", // Buginese
    "khb", // NewTaiLue
    "cu", // Glagolitic
    "shi", // Tifinagh
    "syl", // SylotiNagri
    "peo", // OldPersian
    "pra", // Kharoshthi
    "ban", // Balinese
    "akk", // Cuneiform
    "phn", // Phoenician
    "lzh", // PhagsPa
    "man", // Nko
    "su", // Sundanese
    "lep", // Lepcha
    "sat", // OlChiki
    "vai", // Vai
    "saz", // Saurashtra
    "eky", // KayahLi
    "rej", // Rejang
    "xlc", // Lycian
    "xcr", // Carian
    "xld", // Lydian
    "cjm", // Cham
    "nod", // TaiTham
    "blt", // TaiViet
    "ae", // Avestan
    "egy", // EgyptianHieroglyphs
    "smp", // Samaritan
    "lis", // Lisu
    "bax", // Bamum
    "jv", // Javanese
    "mni", // MeeteiMayek
    "arc", // ImperialAramaic
    "xsa", // OldSouthArabian
    "xpr", // InscriptionalParthian
    "pal", // InscriptionalPahlavi
    "otk", // OldTurkic
    "bh", // Kaithi
    "bbc", // Batak
    "pra", // Brahmi
    "myz", // Mandaic
    "ccp", // Chakma
    "xmr", // MeroiticCursive
    "xmr", // MeroiticHieroglyphs
    "hmd", // Miao
    "sa", // Sharada
    "srb", // SoraSompeng
    "doi"  // Takri
};
enum { SpecialLanguageCount = sizeof(specialLanguages) / sizeof(const char *) };

// this could become a list of all languages used for each writing
// system, instead of using the single most common language.
static const char *languageForWritingSystem[] = {
    0,     // Any
    "en",  // Latin
    "el",  // Greek
    "ru",  // Cyrillic
    "hy",  // Armenian
    "he",  // Hebrew
    "ar",  // Arabic
    "syr", // Syriac
    "div", // Thaana
    "hi",  // Devanagari
    "bn",  // Bengali
    "pa",  // Gurmukhi
    "gu",  // Gujarati
    "or",  // Oriya
    "ta",  // Tamil
    "te",  // Telugu
    "kn",  // Kannada
    "ml",  // Malayalam
    "si",  // Sinhala
    "th",  // Thai
    "lo",  // Lao
    "bo",  // Tibetan
    "my",  // Myanmar
    "ka",  // Georgian
    "km",  // Khmer
    "zh-cn", // SimplifiedChinese
    "zh-tw", // TraditionalChinese
    "ja",  // Japanese
    "ko",  // Korean
    "vi",  // Vietnamese
    0, // Symbol
    "sga", // Ogham
    "non", // Runic
    "man" // N'Ko
};
enum { LanguageCount = sizeof(languageForWritingSystem) / sizeof(const char *) };


// Newer FontConfig let's us sort out fonts that contain certain glyphs, but no
// open type tables for is directly. Do this so we don't pick some strange
// pseudo unicode font
static const char *openType[] = {
    0,     // Any
    0,  // Latin
    0,  // Greek
    0,  // Cyrillic
    0,  // Armenian
    0,  // Hebrew
    0,  // Arabic
    "syrc",  // Syriac
    "thaa",  // Thaana
    "deva",  // Devanagari
    "beng",  // Bengali
    "guru",  // Gurmukhi
    "gurj",  // Gujarati
    "orya",  // Oriya
    "taml",  // Tamil
    "telu",  // Telugu
    "knda",  // Kannada
    "mlym",  // Malayalam
    "sinh",  // Sinhala
    0,  // Thai
    0,  // Lao
    "tibt",  // Tibetan
    "mymr",  // Myanmar
    0,  // Georgian
    "khmr",  // Khmer
    0, // SimplifiedChinese
    0, // TraditionalChinese
    0,  // Japanese
    0,  // Korean
    0,  // Vietnamese
    0, // Symbol
    0, // Ogham
    0, // Runic
    "nko " // N'Ko
};

static const char *getFcFamilyForStyleHint(const QFont::StyleHint style)
{
    const char *stylehint = 0;
    switch (style) {
    case QFont::SansSerif:
        stylehint = "sans-serif";
        break;
    case QFont::Serif:
        stylehint = "serif";
        break;
    case QFont::TypeWriter:
    case QFont::Monospace:
        stylehint = "monospace";
        break;
    case QFont::Cursive:
        stylehint = "cursive";
        break;
    case QFont::Fantasy:
        stylehint = "fantasy";
        break;
    default:
        break;
    }
    return stylehint;
}

Q_GUI_EXPORT void qt_registerAliasToFontFamily(const QString &familyName, const QString &alias);

void QFontconfigDatabase::populateFontDatabase()
{
    FcFontSet  *fonts;

    QString familyName;
    FcChar8 *value = 0;
    int weight_value;
    int slant_value;
    int spacing_value;
    FcChar8 *file_value;
    int indexValue;
    FcChar8 *foundry_value;
    FcChar8 *style_value;
    FcBool scalable;
    FcBool antialias;

    {
        FcObjectSet *os = FcObjectSetCreate();
        FcPattern *pattern = FcPatternCreate();
        const char *properties [] = {
            FC_FAMILY, FC_STYLE, FC_WEIGHT, FC_SLANT,
            FC_SPACING, FC_FILE, FC_INDEX,
            FC_LANG, FC_CHARSET, FC_FOUNDRY, FC_SCALABLE, FC_PIXEL_SIZE, FC_WEIGHT,
            FC_WIDTH,
#if FC_VERSION >= 20297
            FC_CAPABILITY,
#endif
            (const char *)0
        };
        const char **p = properties;
        while (*p) {
            FcObjectSetAdd(os, *p);
            ++p;
        }
        fonts = FcFontList(0, pattern, os);
        FcObjectSetDestroy(os);
        FcPatternDestroy(pattern);
    }

    for (int i = 0; i < fonts->nfont; i++) {
        if (FcPatternGetString(fonts->fonts[i], FC_FAMILY, 0, &value) != FcResultMatch)
            continue;
        //         capitalize(value);
        familyName = QString::fromUtf8((const char *)value);
        slant_value = FC_SLANT_ROMAN;
        weight_value = FC_WEIGHT_REGULAR;
        spacing_value = FC_PROPORTIONAL;
        file_value = 0;
        indexValue = 0;
        scalable = FcTrue;


        if (FcPatternGetInteger (fonts->fonts[i], FC_SLANT, 0, &slant_value) != FcResultMatch)
            slant_value = FC_SLANT_ROMAN;
        if (FcPatternGetInteger (fonts->fonts[i], FC_WEIGHT, 0, &weight_value) != FcResultMatch)
            weight_value = FC_WEIGHT_REGULAR;
        if (FcPatternGetInteger (fonts->fonts[i], FC_SPACING, 0, &spacing_value) != FcResultMatch)
            spacing_value = FC_PROPORTIONAL;
        if (FcPatternGetString (fonts->fonts[i], FC_FILE, 0, &file_value) != FcResultMatch)
            file_value = 0;
        if (FcPatternGetInteger (fonts->fonts[i], FC_INDEX, 0, &indexValue) != FcResultMatch)
            indexValue = 0;
        if (FcPatternGetBool(fonts->fonts[i], FC_SCALABLE, 0, &scalable) != FcResultMatch)
            scalable = FcTrue;
        if (FcPatternGetString(fonts->fonts[i], FC_FOUNDRY, 0, &foundry_value) != FcResultMatch)
            foundry_value = 0;
        if (FcPatternGetString(fonts->fonts[i], FC_STYLE, 0, &style_value) != FcResultMatch)
            style_value = 0;
        if(FcPatternGetBool(fonts->fonts[i],FC_ANTIALIAS,0,&antialias) != FcResultMatch)
            antialias = true;

        QSupportedWritingSystems writingSystems;
        FcLangSet *langset = 0;
        FcResult res = FcPatternGetLangSet(fonts->fonts[i], FC_LANG, 0, &langset);
        if (res == FcResultMatch) {
            bool hasLang = false;
            for (int i = 1; i < LanguageCount; ++i) {
                const FcChar8 *lang = (const FcChar8*) languageForWritingSystem[i];
                if (lang) {
                    FcLangResult langRes = FcLangSetHasLang(langset, lang);
                    if (langRes != FcLangDifferentLang) {
                        writingSystems.setSupported(QFontDatabase::WritingSystem(i));
                        hasLang = true;
                    }
                }
            }
            if (!hasLang)
                // none of our known languages, add it to the other set
                writingSystems.setSupported(QFontDatabase::Other);
        } else {
            // we set Other to supported for symbol fonts. It makes no
            // sense to merge these with other ones, as they are
            // special in a way.
            writingSystems.setSupported(QFontDatabase::Other);
        }


#if FC_VERSION >= 20297
        for (int j = 1; j < LanguageCount; ++j) {
            if (writingSystems.supported(QFontDatabase::WritingSystem(j))
                && requiresOpenType(j) && openType[j]) {
                FcChar8 *cap;
                res = FcPatternGetString (fonts->fonts[i], FC_CAPABILITY, 0, &cap);
                if (res != FcResultMatch || !strstr((const char *)cap, openType[j]))
                    writingSystems.setSupported(QFontDatabase::WritingSystem(j),false);
            }
        }
#endif

        FontFile *fontFile = new FontFile;
        fontFile->fileName = QLatin1String((const char *)file_value);
        fontFile->indexValue = indexValue;

        QFont::Style style = (slant_value == FC_SLANT_ITALIC)
                         ? QFont::StyleItalic
                         : ((slant_value == FC_SLANT_OBLIQUE)
                            ? QFont::StyleOblique
                            : QFont::StyleNormal);
        QFont::Weight weight = QFont::Weight(getFCWeight(weight_value));

        double pixel_size = 0;
        if (!scalable) {
            int width = 100;
            FcPatternGetInteger (fonts->fonts[i], FC_WIDTH, 0, &width);
            FcPatternGetDouble (fonts->fonts[i], FC_PIXEL_SIZE, 0, &pixel_size);
        }

        bool fixedPitch = spacing_value >= FC_MONO;
        QFont::Stretch stretch = QFont::Unstretched;
        QString styleName = style_value ? QString::fromUtf8((const char *) style_value) : QString();
        QPlatformFontDatabase::registerFont(familyName,styleName,QLatin1String((const char *)foundry_value),weight,style,stretch,antialias,scalable,pixel_size,fixedPitch,writingSystems,fontFile);
//        qDebug() << familyName << (const char *)foundry_value << weight << style << &writingSystems << scalable << true << pixel_size;

        for (int k = 1; FcPatternGetString(fonts->fonts[i], FC_FAMILY, k, &value) == FcResultMatch; ++k)
            qt_registerAliasToFontFamily(familyName, QString::fromUtf8((const char *)value));
    }

    FcFontSetDestroy (fonts);

    struct FcDefaultFont {
        const char *qtname;
        const char *rawname;
        bool fixed;
    };
    const FcDefaultFont defaults[] = {
        { "Serif", "serif", false },
        { "Sans Serif", "sans-serif", false },
        { "Monospace", "monospace", true },
        { 0, 0, false }
    };
    const FcDefaultFont *f = defaults;
    // aliases only make sense for 'common', not for any of the specials
    QSupportedWritingSystems ws;
    ws.setSupported(QFontDatabase::Latin);

    while (f->qtname) {
        QString familyQtName = QString::fromLatin1(f->qtname);
        registerFont(familyQtName,QString(),QString(),QFont::Normal,QFont::StyleNormal,QFont::Unstretched,true,true,0,f->fixed,ws,0);
        registerFont(familyQtName,QString(),QString(),QFont::Normal,QFont::StyleItalic,QFont::Unstretched,true,true,0,f->fixed,ws,0);
        registerFont(familyQtName,QString(),QString(),QFont::Normal,QFont::StyleOblique,QFont::Unstretched,true,true,0,f->fixed,ws,0);
        ++f;
    }

    //Lighthouse has very lazy population of the font db. We want it to be initialized when
    //QApplication is constructed, so that the population procedure can do something like this to
    //set the default font
//    const FcDefaultFont *s = defaults;
//    QFont font("Sans Serif");
//    font.setPointSize(9);
//    QApplication::setFont(font);
}

QFontEngineMulti *QFontconfigDatabase::fontEngineMulti(QFontEngine *fontEngine, QChar::Script script)
{
    return new QFontEngineMultiFontConfig(fontEngine, script);
}

QFontEngine *QFontconfigDatabase::fontEngine(const QFontDef &f, QChar::Script script, void *usrPtr)
{
    if (!usrPtr)
        return 0;
    QFontDef fontDef = f;

    QFontEngineFT *engine;
    FontFile *fontfile = static_cast<FontFile *> (usrPtr);
    QFontEngine::FaceId fid;
    fid.filename = QFile::encodeName(fontfile->fileName);
    fid.index = fontfile->indexValue;

    bool antialias = !(fontDef.styleStrategy & QFont::NoAntialias);
    engine = new QFontEngineFT(fontDef);

    QFontEngineFT::GlyphFormat format;
    // try and get the pattern
    FcPattern *pattern = FcPatternCreate();

    FcValue value;
    value.type = FcTypeString;
        QByteArray cs = fontDef.family.toUtf8();
    value.u.s = (const FcChar8 *)cs.data();
    FcPatternAdd(pattern,FC_FAMILY,value,true);

    value.u.s = (const FcChar8 *)fid.filename.data();
    FcPatternAdd(pattern,FC_FILE,value,true);

    value.type = FcTypeInteger;
    value.u.i = fid.index;
    FcPatternAdd(pattern,FC_INDEX,value,true);

    FcResult result;
    FcPattern *match = FcFontMatch(0, pattern, &result);
    if (match) {
        QFontEngineFT::HintStyle default_hint_style;
        if (f.hintingPreference != QFont::PreferDefaultHinting) {
            switch (f.hintingPreference) {
            case QFont::PreferNoHinting:
                default_hint_style = QFontEngineFT::HintNone;
                break;
            case QFont::PreferVerticalHinting:
                default_hint_style = QFontEngineFT::HintLight;
                break;
            case QFont::PreferFullHinting:
            default:
                default_hint_style = QFontEngineFT::HintFull;
                break;
            }
        } else {
            int hint_style = 0;
            if (FcPatternGetInteger (match, FC_HINT_STYLE, 0, &hint_style) == FcResultNoMatch)
                hint_style = QFontEngineFT::HintFull;
            switch (hint_style) {
            case FC_HINT_NONE:
                default_hint_style = QFontEngineFT::HintNone;
                break;
            case FC_HINT_SLIGHT:
                default_hint_style = QFontEngineFT::HintLight;
                break;
            case FC_HINT_MEDIUM:
                default_hint_style = QFontEngineFT::HintMedium;
                break;
            default:
                default_hint_style = QFontEngineFT::HintFull;
                break;
            }
        }

        if (f.hintingPreference == QFont::PreferDefaultHinting) {
            const QPlatformServices *services = QGuiApplicationPrivate::platformIntegration()->services();
            if (services && (services->desktopEnvironment() == "GNOME" || services->desktopEnvironment() == "UNITY")) {
                void *hintStyleResource =
                        QGuiApplication::platformNativeInterface()->nativeResourceForScreen("hintstyle",
                                                                                            QGuiApplication::primaryScreen());
                int hintStyle = int(reinterpret_cast<qintptr>(hintStyleResource));
                if (hintStyle > 0)
                    default_hint_style = QFontEngine::HintStyle(hintStyle - 1);
            }
        }

        engine->setDefaultHintStyle(default_hint_style);

        if (antialias) {
            QFontEngineFT::SubpixelAntialiasingType subpixelType = QFontEngineFT::Subpixel_None;
            int subpixel = FC_RGBA_NONE;

            FcPatternGetInteger(match, FC_RGBA, 0, &subpixel);
            if (subpixel == FC_RGBA_UNKNOWN)
                subpixel = FC_RGBA_NONE;

            switch (subpixel) {
                case FC_RGBA_NONE: subpixelType = QFontEngineFT::Subpixel_None; break;
                case FC_RGBA_RGB: subpixelType = QFontEngineFT::Subpixel_RGB; break;
                case FC_RGBA_BGR: subpixelType = QFontEngineFT::Subpixel_BGR; break;
                case FC_RGBA_VRGB: subpixelType = QFontEngineFT::Subpixel_VRGB; break;
                case FC_RGBA_VBGR: subpixelType = QFontEngineFT::Subpixel_VBGR; break;
                default: break;
            }

            format = subpixelType == QFontEngineFT::Subpixel_None
                        ? QFontEngineFT::Format_A8 : QFontEngineFT::Format_A32;
            engine->subpixelType = subpixelType;
        } else
            format = QFontEngineFT::Format_Mono;

        FcPatternDestroy(match);
    } else
        format = antialias ? QFontEngineFT::Format_A8 : QFontEngineFT::Format_Mono;

    FcPatternDestroy(pattern);

    if (!engine->init(fid,antialias,format)) {
        delete engine;
        engine = 0;
        return engine;
    }
    if (engine->invalid()) {
        delete engine;
        engine = 0;
    } else if (scriptRequiresOpenType(script)) {
        if (!engine->supportsScript(script)) {
            delete engine;
            engine = 0;
        }
    }

    return engine;
}

QStringList QFontconfigDatabase::fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const
{
    QStringList fallbackFamilies;
    FcPattern *pattern = FcPatternCreate();
    if (!pattern)
        return fallbackFamilies;

    FcValue value;
    value.type = FcTypeString;
    QByteArray cs = family.toUtf8();
    value.u.s = (const FcChar8 *)cs.data();
    FcPatternAdd(pattern,FC_FAMILY,value,true);

    int slant_value = FC_SLANT_ROMAN;
    if (style == QFont::StyleItalic)
        slant_value = FC_SLANT_ITALIC;
    else if (style == QFont::StyleOblique)
        slant_value = FC_SLANT_OBLIQUE;
    FcPatternAddInteger(pattern, FC_SLANT, slant_value);

    Q_ASSERT(uint(script) < SpecialLanguageCount);
    if (*specialLanguages[script] != '\0') {
        FcLangSet *ls = FcLangSetCreate();
        FcLangSetAdd(ls, (const FcChar8*)specialLanguages[script]);
        FcPatternAddLangSet(pattern, FC_LANG, ls);
        FcLangSetDestroy(ls);
    } else if (!family.isEmpty()) {
        // If script is Common or Han, then it may include languages like CJK,
        // we should attach system default language set to the pattern
        // to obtain correct font fallback list (i.e. if LANG=zh_CN
        // then we normally want to use a Chinese font for CJK text;
        // while a Japanese font should be used for that if LANG=ja)
        FcPattern *dummy = FcPatternCreate();
        FcDefaultSubstitute(dummy);
        FcChar8 *lang = 0;
        FcResult res = FcPatternGetString(dummy, FC_LANG, 0, &lang);
        if (res == FcResultMatch)
            FcPatternAddString(pattern, FC_LANG, lang);
        FcPatternDestroy(dummy);
    }

    const char *stylehint = getFcFamilyForStyleHint(styleHint);
    if (stylehint) {
        value.u.s = (const FcChar8 *)stylehint;
        FcPatternAddWeak(pattern, FC_FAMILY, value, FcTrue);
    }

    FcConfigSubstitute(0, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcResult result = FcResultMatch;
    FcFontSet *fontSet = FcFontSort(0,pattern,FcFalse,0,&result);
    FcPatternDestroy(pattern);

    if (fontSet) {
        for (int i = 0; i < fontSet->nfont; i++) {
            FcChar8 *value = 0;
            if (FcPatternGetString(fontSet->fonts[i], FC_FAMILY, 0, &value) != FcResultMatch)
                continue;
            //         capitalize(value);
            QString familyName = QString::fromUtf8((const char *)value);
            if (!fallbackFamilies.contains(familyName,Qt::CaseInsensitive) &&
                familyName.compare(family, Qt::CaseInsensitive)) {
                fallbackFamilies << familyName;
            }
        }
        FcFontSetDestroy(fontSet);
    }
//    qDebug() << "fallbackFamilies for:" << family << style << styleHint << script << fallbackFamilies;

    return fallbackFamilies;
}

static FcPattern *queryFont(const FcChar8 *file, const QByteArray &data, int id, FcBlanks *blanks, int *count)
{
#if FC_VERSION < 20402
    Q_UNUSED(data)
    return FcFreeTypeQuery(file, id, blanks, count);
#else
    if (data.isEmpty())
        return FcFreeTypeQuery(file, id, blanks, count);

    FT_Library lib = qt_getFreetype();

    FcPattern *pattern = 0;

    FT_Face face;
    if (!FT_New_Memory_Face(lib, (const FT_Byte *)data.constData(), data.size(), id, &face)) {
        *count = face->num_faces;

        pattern = FcFreeTypeQueryFace(face, file, id, blanks);

        FT_Done_Face(face);
    }

    return pattern;
#endif
}

QStringList QFontconfigDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName)
{
    QStringList families;
    FcFontSet *set = FcConfigGetFonts(0, FcSetApplication);
    if (!set) {
        FcConfigAppFontAddFile(0, (const FcChar8 *)":/non-existent");
        set = FcConfigGetFonts(0, FcSetApplication); // try again
        if (!set)
            return families;
    }

    int id = 0;
    FcBlanks *blanks = FcConfigGetBlanks(0);
    int count = 0;

    FcPattern *pattern = 0;
    do {
        pattern = queryFont((const FcChar8 *)QFile::encodeName(fileName).constData(),
                            fontData, id, blanks, &count);
        if (!pattern)
            return families;

        FcPatternDel(pattern, FC_FILE);
        QByteArray cs = fileName.toUtf8();
        FcPatternAddString(pattern, FC_FILE, (const FcChar8 *) cs.constData());

        FcChar8 *fam = 0;
        if (FcPatternGetString(pattern, FC_FAMILY, 0, &fam) == FcResultMatch) {
            QString family = QString::fromUtf8(reinterpret_cast<const char *>(fam));
            families << family;
        }

        if (!FcFontSetAdd(set, pattern))
            return families;

        ++id;
    } while (pattern && id < count);

    return families;
}

QString QFontconfigDatabase::resolveFontFamilyAlias(const QString &family) const
{
    QString resolved = QBasicFontDatabase::resolveFontFamilyAlias(family);
    if (!resolved.isEmpty() && resolved != family)
        return resolved;
    FcPattern *pattern = FcPatternCreate();
    if (!pattern)
        return family;

    if (!family.isEmpty()) {
        QByteArray cs = family.toUtf8();
        FcPatternAddString(pattern, FC_FAMILY, (const FcChar8 *) cs.constData());
    }
    FcConfigSubstitute(0, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcChar8 *familyAfterSubstitution = 0;
    FcPatternGetString(pattern, FC_FAMILY, 0, &familyAfterSubstitution);
    resolved = QString::fromUtf8((const char *) familyAfterSubstitution);
    FcPatternDestroy(pattern);

    return resolved;
}

QFont QFontconfigDatabase::defaultFont() const
{
    // Hack to get system default language until FcGetDefaultLangs()
    // is exported (https://bugs.freedesktop.org/show_bug.cgi?id=32853)
    // or https://bugs.freedesktop.org/show_bug.cgi?id=35482 is fixed
    FcPattern *dummy = FcPatternCreate();
    FcDefaultSubstitute(dummy);
    FcChar8 *lang = 0;
    FcResult res = FcPatternGetString(dummy, FC_LANG, 0, &lang);

    FcPattern *pattern = FcPatternCreate();
    if (res == FcResultMatch) {
        // Make defaultFont pattern matching locale language aware, because
        // certain FC_LANG based custom rules may happen in FcConfigSubstitute()
        FcPatternAddString(pattern, FC_LANG, lang);
    }
    FcConfigSubstitute(0, pattern, FcMatchPattern);
    FcDefaultSubstitute(pattern);

    FcChar8 *familyAfterSubstitution = 0;
    FcPatternGetString(pattern, FC_FAMILY, 0, &familyAfterSubstitution);
    QString resolved = QString::fromUtf8((const char *) familyAfterSubstitution);
    FcPatternDestroy(pattern);
    FcPatternDestroy(dummy);

    return QFont(resolved);
}

QT_END_NAMESPACE
