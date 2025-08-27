// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qglobal.h"

#include <sys/param.h>

#if defined(Q_OS_MACOS)
#import <AppKit/AppKit.h>
#import <IOKit/graphics/IOGraphicsLib.h>
#elif defined(QT_PLATFORM_UIKIT)
#import <UIKit/UIFont.h>
#endif

#include <QtCore/qelapsedtimer.h>
#include <QtCore/private/qcore_mac_p.h>

#include "qcoretextfontdatabase_p.h"
#include "qfontengine_coretext_p.h"
#if QT_CONFIG(settings)
#include <QtCore/QSettings>
#endif
#include <QtCore/QtEndian>
#ifndef QT_NO_FREETYPE
#include <QtGui/private/qfontengine_ft_p.h>
#endif

#include <QtGui/qpa/qwindowsysteminterface.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QT_IMPL_METATYPE_EXTERN_TAGGED(QCFType<CGFontRef>, QCFType_CGFontRef)
QT_IMPL_METATYPE_EXTERN_TAGGED(QCFType<CFURLRef>, QCFType_CFURLRef)

// this could become a list of all languages used for each writing
// system, instead of using the single most common language.
static const char languageForWritingSystem[][8] = {
    "",    // Any
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
    "zh-Hans", // SimplifiedChinese
    "zh-Hant", // TraditionalChinese
    "ja",  // Japanese
    "ko",  // Korean
    "vi",  // Vietnamese
    "",    // Symbol
    "sga", // Ogham
    "non", // Runic
    "man" // N'Ko
};
enum { LanguageCount = sizeof languageForWritingSystem / sizeof *languageForWritingSystem };

QCoreTextFontDatabase::QCoreTextFontDatabase()
    : m_hasPopulatedAliases(false)
{
#if defined(Q_OS_MACOS)
    m_fontSetObserver = QMacNotificationObserver(nil, NSFontSetChangedNotification, [] {
        qCDebug(lcQpaFonts) << "Fonts have changed";
        QPlatformFontDatabase::repopulateFontDatabase();
    });
#endif
}

QCoreTextFontDatabase::~QCoreTextFontDatabase()
{
    qDeleteAll(m_themeFonts);
}

CTFontDescriptorRef descriptorForFamily(const QString &familyName)
{
    return CTFontDescriptorCreateWithAttributes(CFDictionaryRef(@{
        (id)kCTFontFamilyNameAttribute: familyName.toNSString()
    }));
}
void QCoreTextFontDatabase::populateFontDatabase()
{
    qCDebug(lcQpaFonts) << "Populating font database...";
    QElapsedTimer elapsed;
    if (lcQpaFonts().isDebugEnabled())
        elapsed.start();

    QCFType<CFArrayRef> familyNames = CTFontManagerCopyAvailableFontFamilyNames();
    for (NSString *familyName in familyNames.as<const NSArray *>())
        QPlatformFontDatabase::registerFontFamily(QString::fromNSString(familyName));

    // Some fonts has special handling since macOS Catalina: It is available
    // on the platform, so that it may be used by applications directly, but does not
    // get enumerated. Since there are no alternatives, we hardcode it.
    if (QOperatingSystemVersion::current() >= QOperatingSystemVersion::MacOSCatalina
            && !qEnvironmentVariableIsSet("QT_NO_HARDCODED_FALLBACK_FONTS")) {
        m_hardcodedFallbackFonts[QChar::Script_Adlam] = QStringLiteral("Noto Sans Adlam");
        m_hardcodedFallbackFonts[QChar::Script_Ahom] = QStringLiteral("Noto Serif Ahom");
        m_hardcodedFallbackFonts[QChar::Script_Avestan] = QStringLiteral("Noto Sans Avestan");
        m_hardcodedFallbackFonts[QChar::Script_Balinese] = QStringLiteral("Noto Serif Balinese");
        m_hardcodedFallbackFonts[QChar::Script_Bamum] = QStringLiteral("Noto Sans Bamum");
        m_hardcodedFallbackFonts[QChar::Script_BassaVah] = QStringLiteral("Noto Sans Bassa Vah");
        m_hardcodedFallbackFonts[QChar::Script_Batak] = QStringLiteral("Noto Sans Batak");
        m_hardcodedFallbackFonts[QChar::Script_Bhaiksuki] = QStringLiteral("Noto Sans Bhaiksuki");
        m_hardcodedFallbackFonts[QChar::Script_Brahmi] = QStringLiteral("Noto Sans Brahmi");
        m_hardcodedFallbackFonts[QChar::Script_Buginese] = QStringLiteral("Noto Sans Buginese");
        m_hardcodedFallbackFonts[QChar::Script_Buhid] = QStringLiteral("Noto Sans Buhid");
        m_hardcodedFallbackFonts[QChar::Script_Carian] = QStringLiteral("Noto Sans Carian");
        m_hardcodedFallbackFonts[QChar::Script_CaucasianAlbanian] = QStringLiteral("Noto Sans Caucasian Albanian");
        m_hardcodedFallbackFonts[QChar::Script_Chakma] = QStringLiteral("Noto Sans Chakma");
        m_hardcodedFallbackFonts[QChar::Script_Cham] = QStringLiteral("Noto Sans Cham");
        m_hardcodedFallbackFonts[QChar::Script_Coptic] = QStringLiteral("Noto Sans Coptic");
        m_hardcodedFallbackFonts[QChar::Script_Cuneiform] = QStringLiteral("Noto Sans Cuneiform");
        m_hardcodedFallbackFonts[QChar::Script_Cypriot] = QStringLiteral("Noto Sans Cypriot");
        m_hardcodedFallbackFonts[QChar::Script_Duployan] = QStringLiteral("Noto Sans Duployan");
        m_hardcodedFallbackFonts[QChar::Script_EgyptianHieroglyphs] = QStringLiteral("Noto Sans Egyptian Hieroglyphs");
        m_hardcodedFallbackFonts[QChar::Script_Elbasan] = QStringLiteral("Noto Sans Elbasan");
        m_hardcodedFallbackFonts[QChar::Script_Glagolitic] = QStringLiteral("Noto Sans Glagolitic");
        m_hardcodedFallbackFonts[QChar::Script_Gothic] = QStringLiteral("Noto Sans Gothic");
        m_hardcodedFallbackFonts[QChar::Script_HanifiRohingya] = QStringLiteral("Noto Sans Hanifi Rohingya");
        m_hardcodedFallbackFonts[QChar::Script_Hanunoo] = QStringLiteral("Noto Sans Hanunoo");
        m_hardcodedFallbackFonts[QChar::Script_Hatran] = QStringLiteral("Noto Sans Hatran");
        m_hardcodedFallbackFonts[QChar::Script_ImperialAramaic] = QStringLiteral("Noto Sans Imperial Aramaic");
        m_hardcodedFallbackFonts[QChar::Script_InscriptionalPahlavi] = QStringLiteral("Noto Sans Inscriptional Pahlavi");
        m_hardcodedFallbackFonts[QChar::Script_InscriptionalParthian] = QStringLiteral("Noto Sans Inscriptional Parthian");
        m_hardcodedFallbackFonts[QChar::Script_Javanese] = QStringLiteral("Noto Sans Javanese");
        m_hardcodedFallbackFonts[QChar::Script_Kaithi] = QStringLiteral("Noto Sans Kaithi");
        m_hardcodedFallbackFonts[QChar::Script_KayahLi] = QStringLiteral("Noto Sans Kayah Li");
        m_hardcodedFallbackFonts[QChar::Script_Kharoshthi] = QStringLiteral("Noto Sans Kharoshthi");
        m_hardcodedFallbackFonts[QChar::Script_Khojki] = QStringLiteral("Noto Sans Khojki");
        m_hardcodedFallbackFonts[QChar::Script_Khudawadi] = QStringLiteral("Noto Sans Khudawadi");
        m_hardcodedFallbackFonts[QChar::Script_Lepcha] = QStringLiteral("Noto Sans Lepcha");
        m_hardcodedFallbackFonts[QChar::Script_Limbu] = QStringLiteral("Noto Sans Limbu");
        m_hardcodedFallbackFonts[QChar::Script_LinearA] = QStringLiteral("Noto Sans Linear A");
        m_hardcodedFallbackFonts[QChar::Script_LinearB] = QStringLiteral("Noto Sans Linear B");
        m_hardcodedFallbackFonts[QChar::Script_Lisu] = QStringLiteral("Noto Sans Lisu");
        m_hardcodedFallbackFonts[QChar::Script_Lycian] = QStringLiteral("Noto Sans Lycian");
        m_hardcodedFallbackFonts[QChar::Script_Lydian] = QStringLiteral("Noto Sans Lydian");
        m_hardcodedFallbackFonts[QChar::Script_Mahajani] = QStringLiteral("Noto Sans Mahajani");
        m_hardcodedFallbackFonts[QChar::Script_Mandaic] = QStringLiteral("Noto Sans Mandaic");
        m_hardcodedFallbackFonts[QChar::Script_Manichaean] = QStringLiteral("Noto Sans Manichaean");
        m_hardcodedFallbackFonts[QChar::Script_Marchen] = QStringLiteral("Noto Sans Marchen");
        m_hardcodedFallbackFonts[QChar::Script_MendeKikakui] = QStringLiteral("Noto Sans Mende Kikakui");
        m_hardcodedFallbackFonts[QChar::Script_MeroiticCursive] = QStringLiteral("Noto Sans Meroitic");
        m_hardcodedFallbackFonts[QChar::Script_MeroiticHieroglyphs] = QStringLiteral("Noto Sans Meroitic");
        m_hardcodedFallbackFonts[QChar::Script_Miao] = QStringLiteral("Noto Sans Miao");
        m_hardcodedFallbackFonts[QChar::Script_Modi] = QStringLiteral("Noto Sans Modi");
        m_hardcodedFallbackFonts[QChar::Script_Mongolian] = QStringLiteral("Noto Sans Mongolian");
        m_hardcodedFallbackFonts[QChar::Script_Mro] = QStringLiteral("Noto Sans Mro");
        m_hardcodedFallbackFonts[QChar::Script_MeeteiMayek] = QStringLiteral("Noto Sans Meetei Mayek");
        m_hardcodedFallbackFonts[QChar::Script_Multani] = QStringLiteral("Noto Sans Multani");
        m_hardcodedFallbackFonts[QChar::Script_Nabataean] = QStringLiteral("Noto Sans Nabataean");
        m_hardcodedFallbackFonts[QChar::Script_Newa] = QStringLiteral("Noto Sans Newa");
        m_hardcodedFallbackFonts[QChar::Script_NewTaiLue] = QStringLiteral("Noto Sans New Tai Lue");
        m_hardcodedFallbackFonts[QChar::Script_Nko] = QStringLiteral("Noto Sans Nko");
        m_hardcodedFallbackFonts[QChar::Script_OlChiki] = QStringLiteral("Noto Sans Ol Chiki");
        m_hardcodedFallbackFonts[QChar::Script_OldHungarian] = QStringLiteral("Noto Sans Old Hungarian");
        m_hardcodedFallbackFonts[QChar::Script_OldItalic] = QStringLiteral("Noto Sans Old Italic");
        m_hardcodedFallbackFonts[QChar::Script_OldNorthArabian] = QStringLiteral("Noto Sans Old North Arabian");
        m_hardcodedFallbackFonts[QChar::Script_OldPermic] = QStringLiteral("Noto Sans Old Permic");
        m_hardcodedFallbackFonts[QChar::Script_OldPersian] = QStringLiteral("Noto Sans Old Persian");
        m_hardcodedFallbackFonts[QChar::Script_OldSouthArabian] = QStringLiteral("Noto Sans Old South Arabian");
        m_hardcodedFallbackFonts[QChar::Script_OldTurkic] = QStringLiteral("Noto Sans Old Turkic");
        m_hardcodedFallbackFonts[QChar::Script_Osage] = QStringLiteral("Noto Sans Osage");
        m_hardcodedFallbackFonts[QChar::Script_Osmanya] = QStringLiteral("Noto Sans Osmanya");
        m_hardcodedFallbackFonts[QChar::Script_PahawhHmong] = QStringLiteral("Noto Sans Pahawh Hmong");
        m_hardcodedFallbackFonts[QChar::Script_Palmyrene] = QStringLiteral("Noto Sans Palmyrene");
        m_hardcodedFallbackFonts[QChar::Script_PauCinHau] = QStringLiteral("Noto Sans Pau Cin Hau");
        m_hardcodedFallbackFonts[QChar::Script_PhagsPa] = QStringLiteral("Noto Sans PhagsPa");
        m_hardcodedFallbackFonts[QChar::Script_Phoenician] = QStringLiteral("Noto Sans Phoenician");
        m_hardcodedFallbackFonts[QChar::Script_PsalterPahlavi] = QStringLiteral("Noto Sans Psalter Pahlavi");
        m_hardcodedFallbackFonts[QChar::Script_Rejang] = QStringLiteral("Noto Sans Rejang");
        m_hardcodedFallbackFonts[QChar::Script_Samaritan] = QStringLiteral("Noto Sans Samaritan");
        m_hardcodedFallbackFonts[QChar::Script_Saurashtra] = QStringLiteral("Noto Sans Saurashtra");
        m_hardcodedFallbackFonts[QChar::Script_Sharada] = QStringLiteral("Noto Sans Sharada");
        m_hardcodedFallbackFonts[QChar::Script_Siddham] = QStringLiteral("Noto Sans Siddham");
        m_hardcodedFallbackFonts[QChar::Script_SoraSompeng] = QStringLiteral("Noto Sans Sora Sompeng");
        m_hardcodedFallbackFonts[QChar::Script_Sundanese] = QStringLiteral("Noto Sans Sundanese");
        m_hardcodedFallbackFonts[QChar::Script_SylotiNagri] = QStringLiteral("Noto Sans Syloti Nagri");
        m_hardcodedFallbackFonts[QChar::Script_Tagalog] = QStringLiteral("Noto Sans Tagalog");
        m_hardcodedFallbackFonts[QChar::Script_Tagbanwa] = QStringLiteral("Noto Sans Tagbanwa");
        m_hardcodedFallbackFonts[QChar::Script_Takri] = QStringLiteral("Noto Sans Takri");
        m_hardcodedFallbackFonts[QChar::Script_TaiLe] = QStringLiteral("Noto Sans Tai Le");
        m_hardcodedFallbackFonts[QChar::Script_TaiTham] = QStringLiteral("Noto Sans Tai Tham");
        m_hardcodedFallbackFonts[QChar::Script_TaiViet] = QStringLiteral("Noto Sans Tai Viet");
        m_hardcodedFallbackFonts[QChar::Script_Thaana] = QStringLiteral("Noto Sans Thaana");
        m_hardcodedFallbackFonts[QChar::Script_Tifinagh] = QStringLiteral("Noto Sans Tifinagh");
        m_hardcodedFallbackFonts[QChar::Script_Tirhuta] = QStringLiteral("Noto Sans Tirhuta");
        m_hardcodedFallbackFonts[QChar::Script_Ugaritic] = QStringLiteral("Noto Sans Ugaritic");
        m_hardcodedFallbackFonts[QChar::Script_Vai] = QStringLiteral("Noto Sans Vai");
        m_hardcodedFallbackFonts[QChar::Script_WarangCiti] = QStringLiteral("Noto Sans Warang Citi");
        m_hardcodedFallbackFonts[QChar::Script_Wancho] = QStringLiteral("Noto Sans Wancho");
        m_hardcodedFallbackFonts[QChar::Script_Yi] = QStringLiteral("Noto Sans Yi");
    }

    qCDebug(lcQpaFonts) << "Populating available families took" << elapsed.restart() << "ms";

    populateThemeFonts();

    for (auto familyName : m_systemFontDescriptors.keys()) {
        for (auto fontDescriptor : m_systemFontDescriptors.value(familyName))
            populateFromDescriptor(fontDescriptor, familyName);
    }

    // The font database now has a reference to the original descriptors
    m_systemFontDescriptors.clear();

    qCDebug(lcQpaFonts) << "Populating system descriptors took" << elapsed.restart() << "ms";

    Q_ASSERT(!m_hasPopulatedAliases);
}

bool QCoreTextFontDatabase::populateFamilyAliases(const QString &missingFamily)
{
#if defined(Q_OS_MACOS)
    if (isFamilyPopulated(missingFamily)) {
        // We got here because one of the other properties of the font mismatched,
        // for example the style, so there's no point in populating font aliases.
        return false;
    }

    if (m_hasPopulatedAliases)
        return false;

    // There's no API to go from a localized family name to its non-localized
    // name, so we have to resort to enumerating all the available fonts and
    // doing a reverse lookup.

    qCDebug(lcQpaFonts) << "Populating family aliases...";
    QElapsedTimer elapsed;
    elapsed.start();

    QString nonLocalizedMatch;
    QCFType<CFArrayRef> familyNames = CTFontManagerCopyAvailableFontFamilyNames();
    NSFontManager *fontManager = NSFontManager.sharedFontManager;
    for (NSString *familyName in familyNames.as<const NSArray *>()) {
        NSString *localizedFamilyName = [fontManager localizedNameForFamily:familyName face:nil];
        if (![localizedFamilyName isEqual:familyName]) {
            QString nonLocalizedFamily = QString::fromNSString(familyName);
            QString localizedFamily = QString::fromNSString(localizedFamilyName);
            QPlatformFontDatabase::registerAliasToFontFamily(nonLocalizedFamily, localizedFamily);
            if (localizedFamily == missingFamily)
                nonLocalizedMatch = nonLocalizedFamily;
        }
    }
    m_hasPopulatedAliases = true;

    if (lcQpaFonts().isWarningEnabled()) {
        QString warningMessage;
        QDebug msg(&warningMessage);

        msg << "Populating font family aliases took" << elapsed.restart() << "ms.";
        if (!nonLocalizedMatch.isNull())
            msg << "Replace uses of" << missingFamily << "with its non-localized name" << nonLocalizedMatch;
        else
            msg << "Replace uses of missing font family" << missingFamily << "with one that exists";
        msg << "to avoid this cost.";

        qCWarning(lcQpaFonts) << qPrintable(warningMessage);
    }

    return true;
#else
    Q_UNUSED(missingFamily);
    return false;
#endif
}

CTFontDescriptorRef descriptorForFamily(const char *familyName)
{
    return descriptorForFamily(QString::fromLatin1(familyName));
}

void QCoreTextFontDatabase::populateFamily(const QString &familyName)
{
    qCDebug(lcQpaFonts) << "Populating family" << familyName;

    // A single family might match several different fonts with different styles.
    // We need to add them all so that the font database has the full picture,
    // as once a family has been populated we will not populate it again.
    QCFType<CTFontDescriptorRef> familyDescriptor = descriptorForFamily(familyName);
    QCFType<CFArrayRef> matchingFonts = CTFontDescriptorCreateMatchingFontDescriptors(familyDescriptor, nullptr);
    if (!matchingFonts) {
        qCWarning(lcQpaFonts) << "QCoreTextFontDatabase: Found no matching fonts for family" << familyName;
        return;
    }

    const int numFonts = CFArrayGetCount(matchingFonts);
    for (int i = 0; i < numFonts; ++i)
        populateFromDescriptor(CTFontDescriptorRef(CFArrayGetValueAtIndex(matchingFonts, i)), familyName);
}

void QCoreTextFontDatabase::invalidate()
{
    qCDebug(lcQpaFonts) << "Invalidating font database";
    m_hasPopulatedAliases = false;

    qDeleteAll(m_themeFonts);
    m_themeFonts.clear();
    QWindowSystemInterface::handleThemeChange<QWindowSystemInterface::SynchronousDelivery>();
}

struct FontDescription {
    QCFString familyName;
    QCFString styleName;
    QString foundryName;
    QFont::Weight weight;
    QFont::Style style;
    QFont::Stretch stretch;
    qreal pointSize;
    bool fixedPitch;
    bool colorFont;
    QSupportedWritingSystems writingSystems;
};

#ifndef QT_NO_DEBUG_STREAM
Q_DECL_UNUSED static inline QDebug operator<<(QDebug debug, const FontDescription &fd)
{
    QDebugStateSaver saver(debug);
    return debug.nospace() << "FontDescription("
        << "familyName=" << QString(fd.familyName)
        << ", styleName=" << QString(fd.styleName)
        << ", foundry=" << fd.foundryName
        << ", weight=" << fd.weight
        << ", style=" << fd.style
        << ", stretch=" << fd.stretch
        << ", pointSize=" << fd.pointSize
        << ", fixedPitch=" << fd.fixedPitch
        << ", writingSystems=" << fd.writingSystems
    << ")";
}
#endif

static void getFontDescription(CTFontDescriptorRef font, FontDescription *fd)
{
    QCFType<CFDictionaryRef> styles = (CFDictionaryRef) CTFontDescriptorCopyAttribute(font, kCTFontTraitsAttribute);

    fd->foundryName = QStringLiteral("CoreText");
    fd->familyName = (CFStringRef) CTFontDescriptorCopyAttribute(font, kCTFontFamilyNameAttribute);
    fd->styleName = (CFStringRef)CTFontDescriptorCopyAttribute(font, kCTFontStyleNameAttribute);
    fd->weight = QFont::Normal;
    fd->style = QFont::StyleNormal;
    fd->stretch = QFont::Unstretched;
    fd->fixedPitch = false;
    fd->colorFont = false;



    if (QCFType<CTFontRef> tempFont = CTFontCreateWithFontDescriptor(font, 0.0, 0)) {
        uint tag = QFont::Tag("OS/2").value();
        CTFontRef tempFontRef = tempFont;
        void *userData = reinterpret_cast<void *>(&tempFontRef);
        uint length = 128;
        QVarLengthArray<uchar, 128> os2Table(length);
        if (QCoreTextFontEngine::ct_getSfntTable(userData, tag, os2Table.data(), &length) && length >= 86) {
            if (length > uint(os2Table.length())) {
                os2Table.resize(length);
                if (!QCoreTextFontEngine::ct_getSfntTable(userData, tag, os2Table.data(), &length))
                    Q_UNREACHABLE();
                Q_ASSERT(length >= 86);
            }
            fd->writingSystems = QPlatformFontDatabase::writingSystemsFromOS2Table(reinterpret_cast<const char *>(os2Table.data()), length);
        }
    }

    if (styles) {
        if (CFNumberRef weightValue = (CFNumberRef) CFDictionaryGetValue(styles, kCTFontWeightTrait)) {
            double normalizedWeight;
            if (CFNumberGetValue(weightValue, kCFNumberFloat64Type, &normalizedWeight))
                fd->weight = QCoreTextFontEngine::qtWeightFromCFWeight(float(normalizedWeight));
        }
        if (CFNumberRef italic = (CFNumberRef) CFDictionaryGetValue(styles, kCTFontSlantTrait)) {
            double d;
            if (CFNumberGetValue(italic, kCFNumberDoubleType, &d)) {
                if (d > 0.0)
                    fd->style = QFont::StyleItalic;
            }
        }
        if (CFNumberRef symbolic = (CFNumberRef) CFDictionaryGetValue(styles, kCTFontSymbolicTrait)) {
            int d;
            if (CFNumberGetValue(symbolic, kCFNumberSInt32Type, &d)) {
                if (d & kCTFontColorGlyphsTrait)
                    fd->colorFont = true;

                if (d & kCTFontMonoSpaceTrait)
                    fd->fixedPitch = true;
                if (d & kCTFontExpandedTrait)
                    fd->stretch = QFont::Expanded;
                else if (d & kCTFontCondensedTrait)
                    fd->stretch = QFont::Condensed;
            }
        }
    }

    if (QCFType<CFNumberRef> size = (CFNumberRef) CTFontDescriptorCopyAttribute(font, kCTFontSizeAttribute)) {
        if (CFNumberIsFloatType(size)) {
            double d;
            CFNumberGetValue(size, kCFNumberDoubleType, &d);
            fd->pointSize = d;
        } else {
            int i;
            CFNumberGetValue(size, kCFNumberIntType, &i);
            fd->pointSize = i;
        }
    }

    if (QCFType<CFArrayRef> languages = (CFArrayRef) CTFontDescriptorCopyAttribute(font, kCTFontLanguagesAttribute)) {
        CFIndex length = CFArrayGetCount(languages);
        for (int i = 1; i < LanguageCount; ++i) {
            if (!*languageForWritingSystem[i])
                continue;
            QCFString lang = CFStringCreateWithCString(NULL, languageForWritingSystem[i], kCFStringEncodingASCII);
            if (CFArrayContainsValue(languages, CFRangeMake(0, length), lang))
                fd->writingSystems.setSupported(QFontDatabase::WritingSystem(i));
        }
    }
}

void QCoreTextFontDatabase::populateFromDescriptor(CTFontDescriptorRef font, const QString &familyName, QFontDatabasePrivate::ApplicationFont *applicationFont)
{
    FontDescription fd;
    getFontDescription(font, &fd);

    // Note: The familyName we are registering, and the family name of the font descriptor, may not
    // match, as CTFontDescriptorCreateMatchingFontDescriptors will return descriptors for replacement
    // fonts if a font family does not have any fonts available on the system.
    QString family = !familyName.isNull() ? familyName : static_cast<QString>(fd.familyName);

    if (applicationFont != nullptr) {
        QFontDatabasePrivate::ApplicationFont::Properties properties;
        properties.familyName = family;
        properties.styleName = fd.styleName;
        properties.weight = fd.weight;
        properties.stretch = fd.stretch;
        properties.style = fd.style;

        applicationFont->properties.append(properties);
    }

    CFRetain(font);
    QPlatformFontDatabase::registerFont(family, fd.styleName, fd.foundryName, fd.weight, fd.style, fd.stretch,
            true /* antialiased */, true /* scalable */, 0 /* pixelSize, ignored as font is scalable */,
            fd.fixedPitch, fd.colorFont, fd.writingSystems, (void *)font);
}

static NSString * const kQtFontDataAttribute = @"QtFontDataAttribute";

template <typename T>
T *descriptorAttribute(CTFontDescriptorRef descriptor, CFStringRef name)
{
    return [static_cast<T *>(CTFontDescriptorCopyAttribute(descriptor, name)) autorelease];
}

void QCoreTextFontDatabase::releaseHandle(void *handle)
{
    CTFontDescriptorRef descriptor = static_cast<CTFontDescriptorRef>(handle);
    if (NSValue *fontDataValue = descriptorAttribute<NSValue>(descriptor, (CFStringRef)kQtFontDataAttribute)) {
        QByteArray *fontData = static_cast<QByteArray *>(fontDataValue.pointerValue);
        delete fontData;
    }
    CFRelease(descriptor);
}

extern CGAffineTransform qt_transform_from_fontdef(const QFontDef &fontDef);

template <>
QFontEngine *QCoreTextFontDatabaseEngineFactory<QCoreTextFontEngine>::fontEngine(const QFontDef &fontDef, void *usrPtr)
{
    QCFType<CTFontDescriptorRef> descriptor = QCFType<CTFontDescriptorRef>::constructFromGet(
        static_cast<CTFontDescriptorRef>(usrPtr));

    // Since we do not pass in the destination DPI to CoreText when making
    // the font, we need to pass in a point size which is scaled to include
    // the DPI. The default DPI for the screen is 72, thus the scale factor
    // is destinationDpi / 72, but since pixelSize = pointSize / 72 * dpi,
    // the pixelSize is actually the scaled point size for the destination
    // DPI, and we can use that directly.
    qreal scaledPointSize = fontDef.pixelSize;

    CGAffineTransform matrix = qt_transform_from_fontdef(fontDef);

    if (!fontDef.variableAxisValues.isEmpty()) {
        QCFType<CFMutableDictionaryRef> variations = CFDictionaryCreateMutable(nullptr,
                                                                       fontDef.variableAxisValues.size(),
                                                                       &kCFTypeDictionaryKeyCallBacks,
                                                                       &kCFTypeDictionaryValueCallBacks);
        for (auto it = fontDef.variableAxisValues.constBegin();
             it != fontDef.variableAxisValues.constEnd();
             ++it) {
            const quint32 tag = it.key().value();
            const float value = it.value();
            QCFType<CFNumberRef> tagRef = CFNumberCreate(nullptr, kCFNumberIntType, &tag);
            QCFType<CFNumberRef> valueRef = CFNumberCreate(nullptr, kCFNumberFloatType, &value);

            CFDictionarySetValue(variations, tagRef, valueRef);
        }
        QCFType<CFDictionaryRef> attributes = CFDictionaryCreate(nullptr,
                                                                 (const void **) &kCTFontVariationAttribute,
                                                                 (const void **) &variations,
                                                                 1,
                                                                 &kCFTypeDictionaryKeyCallBacks,
                                                                 &kCFTypeDictionaryValueCallBacks);
        descriptor = CTFontDescriptorCreateCopyWithAttributes(descriptor, attributes);
    }

    if (QCFType<CTFontRef> font = CTFontCreateWithFontDescriptor(descriptor, scaledPointSize, &matrix))
        return new QCoreTextFontEngine(font, fontDef);

    return nullptr;
}

#ifndef QT_NO_FREETYPE
template <>
QFontEngine *QCoreTextFontDatabaseEngineFactory<QFontEngineFT>::fontEngine(const QFontDef &fontDef, void *usrPtr)
{
    CTFontDescriptorRef descriptor = static_cast<CTFontDescriptorRef>(usrPtr);

    if (NSValue *fontDataValue = descriptorAttribute<NSValue>(descriptor, (CFStringRef)kQtFontDataAttribute)) {
        QByteArray *fontData = static_cast<QByteArray *>(fontDataValue.pointerValue);
        return QFontEngineFT::create(*fontData, fontDef.pixelSize,
            static_cast<QFont::HintingPreference>(fontDef.hintingPreference), fontDef.variableAxisValues);
    } else if (NSURL *url = descriptorAttribute<NSURL>(descriptor, kCTFontURLAttribute)) {
        QFontEngine::FaceId faceId;

        Q_ASSERT(url.fileURL);
        QString faceFileName{QString::fromNSString(url.path)};
        faceId.filename = faceFileName.toUtf8();

        QString styleName = QCFString(CTFontDescriptorCopyAttribute(descriptor, kCTFontStyleNameAttribute));
        faceId.index = QFreetypeFace::getFaceIndexByStyleName(faceFileName, styleName);

        faceId.variableAxes = fontDef.variableAxisValues;

        return QFontEngineFT::create(fontDef, faceId);
    }
    // We end up here with a descriptor does not contain Qt font data or kCTFontURLAttribute.
    // Since the FT engine can't deal with a descriptor with just a NSFontNameAttribute,
    // we should return nullptr.
    return nullptr;
}
#endif

template <class T>
QFontEngine *QCoreTextFontDatabaseEngineFactory<T>::fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference)
{
    return T::create(fontData, pixelSize, hintingPreference, {});
}

// Explicitly instantiate so that we don't need the plugin to involve FreeType
template class QCoreTextFontDatabaseEngineFactory<QCoreTextFontEngine>;
#ifndef QT_NO_FREETYPE
template class QCoreTextFontDatabaseEngineFactory<QFontEngineFT>;
#endif

CFArrayRef fallbacksForDescriptor(CTFontDescriptorRef descriptor)
{
    QCFType<CTFontRef> font = CTFontCreateWithFontDescriptor(descriptor, 0.0, nullptr);
    if (!font) {
        qCWarning(lcQpaFonts) << "Failed to create fallback font for" << descriptor;
        return nullptr;
    }

    CFArrayRef cascadeList = CFArrayRef(CTFontCopyDefaultCascadeListForLanguages(font,
        (CFArrayRef)NSLocale.preferredLanguages));

    if (!cascadeList) {
        qCWarning(lcQpaFonts) << "Failed to create fallback cascade list for" << descriptor;
        return nullptr;
    }

    return cascadeList;
}

CFArrayRef QCoreTextFontDatabase::fallbacksForFamily(const QString &family)
{
    if (family.isEmpty())
        return nullptr;

    QCFType<CTFontDescriptorRef> fontDescriptor = descriptorForFamily(family);
    if (!fontDescriptor) {
        qCWarning(lcQpaFonts) << "Failed to create fallback font descriptor for" << family;
        return nullptr;
    }

    // If the font is not available we want to fall back to the style hint.
    // By creating a matching font descriptor we can verify whether the font
    // is available or not, and avoid CTFontCreateWithFontDescriptor picking
    // a default font for us based on incomplete information.
    fontDescriptor = CTFontDescriptorCreateMatchingFontDescriptor(fontDescriptor, 0);
    if (!fontDescriptor)
        return nullptr;

    return fallbacksForDescriptor(fontDescriptor);
}

CTFontDescriptorRef descriptorForFontType(CTFontUIFontType uiType)
{
    static const CGFloat kDefaultSizeForRequestedUIType = 0.0;
    QCFType<CTFontRef> ctFont = CTFontCreateUIFontForLanguage(
        uiType, kDefaultSizeForRequestedUIType, nullptr);
    return CTFontCopyFontDescriptor(ctFont);
}

CTFontDescriptorRef descriptorForStyle(QFont::StyleHint styleHint)
{
    switch (styleHint) {
        case QFont::SansSerif: return descriptorForFamily("Helvetica");
        case QFont::Serif: return descriptorForFamily("Times New Roman");
        case QFont::Monospace: return descriptorForFamily("Menlo");
#ifdef Q_OS_MACOS
        case QFont::Cursive: return descriptorForFamily("Apple Chancery");
#endif
        case QFont::Fantasy: return descriptorForFamily("Zapfino");
        case QFont::TypeWriter: return descriptorForFamily("American Typewriter");
        case QFont::AnyStyle: Q_FALLTHROUGH();
        case QFont::System: return descriptorForFontType(kCTFontUIFontSystem);
        default: return nullptr; // No matching font on this platform
    }
}

QStringList QCoreTextFontDatabase::fallbacksForScript(QFontDatabasePrivate::ExtendedScript script) const
{
    if (script == QFontDatabasePrivate::Script_Emoji)
        return QStringList{} << QStringLiteral(".Apple Color Emoji UI");
    else
        return QStringList{};
}

QStringList QCoreTextFontDatabase::fallbacksForFamily(const QString &family,
                                                      QFont::Style style,
                                                      QFont::StyleHint styleHint,
                                                      QFontDatabasePrivate::ExtendedScript script) const
{
    Q_UNUSED(style);

    qCDebug(lcQpaFonts).nospace() << "Resolving fallbacks families for"
        << (!family.isEmpty() ? qPrintable(" family '%1' with"_L1.arg(family)) : "")
        << " style hint " << styleHint;

    QMacAutoReleasePool pool;

    QStringList fallbackList = fallbacksForScript(script);

    QCFType<CFArrayRef> fallbackFonts = fallbacksForFamily(family);
    if (!fallbackFonts || !CFArrayGetCount(fallbackFonts)) {
        // We were not able to find a fallback for the specific family,
        // or the family was empty, so we fall back to the style hint.
        if (!family.isEmpty())
            qCDebug(lcQpaFonts) << "No fallbacks found. Using style hint instead";

        if (QCFType<CTFontDescriptorRef> styleDescriptor = descriptorForStyle(styleHint)) {
            CFMutableArrayRef tmp = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
            CFArrayAppendValue(tmp, styleDescriptor);
            QCFType<CFArrayRef> styleFallbacks = fallbacksForDescriptor(styleDescriptor);
            CFArrayAppendArray(tmp, styleFallbacks, CFRangeMake(0, CFArrayGetCount(styleFallbacks)));
            fallbackFonts = tmp;
        }
    }

    if (!fallbackFonts)
        return fallbackList;

    const int numberOfFallbacks = CFArrayGetCount(fallbackFonts);
    for (int i = 0; i < numberOfFallbacks; ++i) {
        auto fallbackDescriptor = CTFontDescriptorRef(CFArrayGetValueAtIndex(fallbackFonts, i));
        auto fallbackFamilyName = QCFString(CTFontDescriptorCopyAttribute(fallbackDescriptor, kCTFontFamilyNameAttribute));

        if (!isFamilyPopulated(fallbackFamilyName)) {
            // We need to populate, or at least register the fallback fonts,
            // otherwise the Qt font database may not know they exist.
            if (isPrivateFontFamily(fallbackFamilyName))
                const_cast<QCoreTextFontDatabase *>(this)->populateFromDescriptor(fallbackDescriptor);
            else
                registerFontFamily(fallbackFamilyName);
        }

        fallbackList.append(fallbackFamilyName);
    }

    // Some fallback fonts will have have an order in the list returned
    // by Core Text that would indicate they should be preferred for e.g.
    // Arabic, or Emoji, while in reality only supporting a tiny subset
    // of the required glyphs, or representing them by question marks.
    // Move these to the end, so that the proper fonts are preferred.
    for (const char *family : { ".Apple Symbols Fallback", ".Noto Sans Universal" }) {
        int index = fallbackList.indexOf(QLatin1StringView(family));
        if (index >= 0)
            fallbackList.move(index, fallbackList.size() - 1);
    }

#if defined(Q_OS_MACOS)
    // Since we are only returning a list of default fonts for the current language, we do not
    // cover all Unicode completely. This was especially an issue for some of the common script
    // symbols such as mathematical symbols, currency or geometric shapes. To minimize the risk
    // of missing glyphs, we add Arial Unicode MS as a final fail safe, since this covers most
    // of Unicode 2.1.
    if (!fallbackList.contains(QStringLiteral("Arial Unicode MS")))
        fallbackList.append(QStringLiteral("Arial Unicode MS"));
    // Since some symbols (specifically Braille) are not in Arial Unicode MS, we
    // add Apple Symbols to cover those too.
    if (!fallbackList.contains(QStringLiteral("Apple Symbols")))
        fallbackList.append(QStringLiteral("Apple Symbols"));
    // Some Noto* fonts are not automatically enumerated by system, despite being the main
    // fonts for their writing system.
    if (script < int(QChar::ScriptCount)) {
      QString hardcodedFont = m_hardcodedFallbackFonts.value(QChar::Script(script));
      if (!hardcodedFont.isEmpty() && !fallbackList.contains(hardcodedFont)) {
            if (!isFamilyPopulated(hardcodedFont)) {
                if (!m_privateFamilies.contains(hardcodedFont)) {
                    QCFType<CTFontDescriptorRef> familyDescriptor = descriptorForFamily(hardcodedFont);
                    QCFType<CFArrayRef> matchingFonts = CTFontDescriptorCreateMatchingFontDescriptors(familyDescriptor, nullptr);
                    if (matchingFonts) {
                        const int numFonts = CFArrayGetCount(matchingFonts);
                        for (int i = 0; i < numFonts; ++i)
                            const_cast<QCoreTextFontDatabase *>(this)->populateFromDescriptor(CTFontDescriptorRef(CFArrayGetValueAtIndex(matchingFonts, i)),
                                                                                            hardcodedFont);

                        fallbackList.append(hardcodedFont);
                    }

                    // Register as private family even if the font is not found, in order to avoid
                    // redoing the check later. In later calls, the font will then just be ignored.
                    m_privateFamilies.insert(hardcodedFont);
                }
            } else {
              fallbackList.append(hardcodedFont);
            }
        }
    }
#endif

    extern QStringList qt_sort_families_by_writing_system(QFontDatabasePrivate::ExtendedScript, const QStringList &);
    fallbackList = qt_sort_families_by_writing_system(script, fallbackList);

    qCDebug(lcQpaFonts).nospace() << "Fallback families ordered by script " << script << ": " << fallbackList;

    return fallbackList;
}

QStringList QCoreTextFontDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName, QFontDatabasePrivate::ApplicationFont *applicationFont)
{
    QCFType<CFArrayRef> fonts;

    if (!fontData.isEmpty()) {
        QCFType<CFDataRef> fontDataReference = fontData.toRawCFData();
        if (QCFType<CFArrayRef> descriptors = CTFontManagerCreateFontDescriptorsFromData(fontDataReference)) {
            CFMutableArrayRef array = CFArrayCreateMutable(kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks);
            const int count = CFArrayGetCount(descriptors);

            for (int i = 0; i < count; ++i) {
                CTFontDescriptorRef descriptor = CTFontDescriptorRef(CFArrayGetValueAtIndex(descriptors, i));

                // There's no way to get the data back out of a font descriptor created with
                // CTFontManagerCreateFontDescriptorFromData, so we attach the data manually.
                NSDictionary *attributes = @{ kQtFontDataAttribute : [NSValue valueWithPointer:new QByteArray(fontData)] };
                QCFType<CTFontDescriptorRef> copiedDescriptor = CTFontDescriptorCreateCopyWithAttributes(descriptor, (CFDictionaryRef)attributes);
                CFArrayAppendValue(array, copiedDescriptor);
            }

            fonts = array;
        }
    } else {
        QCFType<CFURLRef> fontURL = QUrl::fromLocalFile(fileName).toCFURL();
        fonts = CTFontManagerCreateFontDescriptorsFromURL(fontURL);
    }

    if (!fonts)
        return QStringList();

    QStringList families;
    const int numFonts = CFArrayGetCount(fonts);
    for (int i = 0; i < numFonts; ++i) {
        CTFontDescriptorRef fontDescriptor = CTFontDescriptorRef(CFArrayGetValueAtIndex(fonts, i));
        populateFromDescriptor(fontDescriptor, QString(), applicationFont);
        QCFType<CFStringRef> familyName = CFStringRef(CTFontDescriptorCopyAttribute(fontDescriptor, kCTFontFamilyNameAttribute));
        families.append(QString::fromCFString(familyName));
    }

    // Note: We don't do font matching via CoreText for application fonts, so we don't
    // need to enable font matching for them via CTFontManagerEnableFontDescriptors.

    return families;
}

bool QCoreTextFontDatabase::isPrivateFontFamily(const QString &family) const
{
    if (family.startsWith(u'.') || family == "LastResort"_L1 || m_privateFamilies.contains(family))
        return true;

    return QPlatformFontDatabase::isPrivateFontFamily(family);
}

static CTFontUIFontType fontTypeFromTheme(QPlatformTheme::Font f)
{
    switch (f) {
    case QPlatformTheme::SystemFont:
        return kCTFontUIFontSystem;

    case QPlatformTheme::MenuFont:
    case QPlatformTheme::MenuBarFont:
    case QPlatformTheme::MenuItemFont:
        return kCTFontUIFontMenuItem;

    case QPlatformTheme::MessageBoxFont:
        return kCTFontUIFontEmphasizedSystem;

    case QPlatformTheme::LabelFont:
        return kCTFontUIFontSystem;

    case QPlatformTheme::TipLabelFont:
        return kCTFontUIFontToolTip;

    case QPlatformTheme::StatusBarFont:
        return kCTFontUIFontSystem;

    case QPlatformTheme::TitleBarFont:
        return kCTFontUIFontWindowTitle;

    case QPlatformTheme::MdiSubWindowTitleFont:
        return kCTFontUIFontSystem;

    case QPlatformTheme::DockWidgetTitleFont:
        return kCTFontUIFontSmallSystem;

    case QPlatformTheme::PushButtonFont:
        return kCTFontUIFontPushButton;

    case QPlatformTheme::CheckBoxFont:
    case QPlatformTheme::RadioButtonFont:
        return kCTFontUIFontSystem;

    case QPlatformTheme::ToolButtonFont:
        return kCTFontUIFontSmallToolbar;

    case QPlatformTheme::ItemViewFont:
        return kCTFontUIFontSystem;

    case QPlatformTheme::ListViewFont:
        return kCTFontUIFontViews;

    case QPlatformTheme::HeaderViewFont:
        return kCTFontUIFontSmallSystem;

    case QPlatformTheme::ListBoxFont:
        return kCTFontUIFontViews;

    case QPlatformTheme::ComboMenuItemFont:
        return kCTFontUIFontSystem;

    case QPlatformTheme::ComboLineEditFont:
        return kCTFontUIFontViews;

    case QPlatformTheme::SmallFont:
        return kCTFontUIFontSmallSystem;

    case QPlatformTheme::MiniFont:
        return kCTFontUIFontMiniSystem;

    case QPlatformTheme::FixedFont:
        return kCTFontUIFontUserFixedPitch;

    default:
        return kCTFontUIFontSystem;
    }
}

static CTFontDescriptorRef fontDescriptorFromTheme(QPlatformTheme::Font f)
{
#if defined(QT_PLATFORM_UIKIT)
    // Use Dynamic Type to resolve theme fonts if possible, to get
    // correct font sizes and style based on user configuration.
    NSString *textStyle = 0;
    switch (f) {
    case QPlatformTheme::TitleBarFont:
    case QPlatformTheme::HeaderViewFont:
        textStyle = UIFontTextStyleHeadline;
        break;
    case QPlatformTheme::MdiSubWindowTitleFont:
        textStyle = UIFontTextStyleSubheadline;
        break;
    case QPlatformTheme::TipLabelFont:
    case QPlatformTheme::SmallFont:
        textStyle = UIFontTextStyleFootnote;
        break;
    case QPlatformTheme::MiniFont:
        textStyle = UIFontTextStyleCaption2;
        break;
    case QPlatformTheme::FixedFont:
        // Fall back to regular code path, as iOS doesn't provide
        // an appropriate text style for this theme font.
        break;
    default:
        textStyle = UIFontTextStyleBody;
        break;
    }

    if (textStyle) {
        UIFontDescriptor *desc = [UIFontDescriptor preferredFontDescriptorWithTextStyle:textStyle];
        return static_cast<CTFontDescriptorRef>(CFBridgingRetain(desc));
    }
#endif // QT_PLATFORM_UIKIT

    // macOS default case and iOS fallback case
    return descriptorForFontType(fontTypeFromTheme(f));
}

void QCoreTextFontDatabase::populateThemeFonts()
{
    QMacAutoReleasePool pool;

    if (!m_themeFonts.isEmpty())
        return;

    QElapsedTimer elapsed;
    if (lcQpaFonts().isDebugEnabled())
        elapsed.start();

    qCDebug(lcQpaFonts) << "Populating theme fonts...";

    for (long f = QPlatformTheme::SystemFont; f < QPlatformTheme::NFonts; f++) {
        QPlatformTheme::Font themeFont = static_cast<QPlatformTheme::Font>(f);
        QCFType<CTFontDescriptorRef> fontDescriptor = fontDescriptorFromTheme(themeFont);
        FontDescription fd;
        getFontDescription(fontDescriptor, &fd);

        // We might get here from QFontDatabase::systemFont() or QPlatformTheme::font(),
        // before the font database has initialized itself and populated all available
        // families. As a result, we can't populate the descriptor at this time, as that
        // would result in the font database having > 0 families, which would result in
        // skipping the initialization and population of all other font families. Instead
        // we store the descriptors for later and populate them during populateFontDatabase().

        bool haveRegisteredFamily = m_systemFontDescriptors.contains(fd.familyName);
        qCDebug(lcQpaFonts) << "Got" << (haveRegisteredFamily ? "already registered" : "unseen")
                            << "family" << fd.familyName << "for" << themeFont;

        if (!haveRegisteredFamily) {
            // We need to register all weights and variants of the theme font,
            // as the user might tweak the returned QFont before use.
            QList<QCFType<CTFontDescriptorRef>> themeFontVariants;

            auto addFontVariants = [&](CTFontDescriptorRef descriptor) {
                QCFType<CFArrayRef> matchingDescriptors = CTFontDescriptorCreateMatchingFontDescriptors(descriptor, nullptr);
                const int matchingDescriptorsCount = matchingDescriptors ? CFArrayGetCount(matchingDescriptors) : 0;
                qCDebug(lcQpaFonts) << "Enumerating font variants based on" << id(descriptor)
                    << "resulted in" << matchingDescriptorsCount << "matching descriptors"
                    << matchingDescriptors.as<NSArray*>();

                for (int i = 0; i < matchingDescriptorsCount; ++i) {
                    auto matchingDescriptor = CTFontDescriptorRef(CFArrayGetValueAtIndex(matchingDescriptors, i));
                    themeFontVariants.append(QCFType<CTFontDescriptorRef>::constructFromGet(matchingDescriptor));
                }
            };

            // Try populating the font variants based on its UI design trait, if available
            auto fontTraits = QCFType<CFDictionaryRef>(CTFontDescriptorCopyAttribute(fontDescriptor, kCTFontTraitsAttribute));
            static const NSString *kUIFontDesignTrait = @"NSCTFontUIFontDesignTrait";
            if (id uiFontDesignTrait = fontTraits.as<NSDictionary*>()[kUIFontDesignTrait]) {
                QCFType<CTFontDescriptorRef> designTraitDescriptor = CTFontDescriptorCreateWithAttributes(
                    CFDictionaryRef(@{ (id)kCTFontTraitsAttribute: @{ kUIFontDesignTrait: uiFontDesignTrait }
                }));
                addFontVariants(designTraitDescriptor);
            }

            if (themeFontVariants.isEmpty()) {
                // Fall back to populating variants based on the family name alone
                QCFType<CTFontDescriptorRef> familyDescriptor = descriptorForFamily(fd.familyName);
                addFontVariants(familyDescriptor);
            }

            if (themeFontVariants.isEmpty()) {
                qCDebug(lcQpaFonts) << "No theme font variants found, falling back to single variant descriptor";
                themeFontVariants.append(fontDescriptor);
            }

            m_systemFontDescriptors.insert(fd.familyName, themeFontVariants);
        }

        QFont *font = new QFont(fd.familyName, fd.pointSize, fd.weight, fd.style == QFont::StyleItalic);
        m_themeFonts.insert(themeFont, font);
    }

    qCDebug(lcQpaFonts) << "Populating theme fonts took" << elapsed.restart() << "ms";
}

QFont *QCoreTextFontDatabase::themeFont(QPlatformTheme::Font f) const
{
    // The code paths via QFontDatabase::systemFont() or QPlatformTheme::font()
    // do not ensure that the font database has been populated, so we need to
    // manually populate the theme fonts lazily here just in case.
    const_cast<QCoreTextFontDatabase*>(this)->populateThemeFonts();

    return m_themeFonts.value(f, nullptr);
}

QFont QCoreTextFontDatabase::defaultFont() const
{
    return QFont(*themeFont(QPlatformTheme::SystemFont));
}

bool QCoreTextFontDatabase::fontsAlwaysScalable() const
{
    return true;
}

QList<int> QCoreTextFontDatabase::standardSizes() const
{
    QList<int> ret;
    static const unsigned short standard[] =
        { 9, 10, 11, 12, 13, 14, 18, 24, 36, 48, 64, 72, 96, 144, 288, 0 };
    ret.reserve(int(sizeof(standard) / sizeof(standard[0])));
    const unsigned short *sizes = standard;
    while (*sizes) ret << *sizes++;
    return ret;
}

bool QCoreTextFontDatabase::supportsColrv0Fonts() const
{
    return true;
}

bool QCoreTextFontDatabase::supportsVariableApplicationFonts() const
{
    return true;
}

QT_END_NAMESPACE

