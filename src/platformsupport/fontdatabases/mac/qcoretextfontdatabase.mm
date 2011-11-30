/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qcoretextfontdatabase_p.h"
#include "qfontengine_coretext_p.h"
#include <QtCore/QSettings>
#import <Foundation/Foundation.h>

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
    0, // Ogham
    0, // Runic
    0 // N'Ko
};
enum { LanguageCount = sizeof(languageForWritingSystem) / sizeof(const char *) };

inline QString qt_mac_NSStringToQString(const NSString *nsstr)
{ return QCFString::toQString(reinterpret_cast<const CFStringRef>(nsstr)); }

int qt_antialiasing_threshold = 0;

QFont::StyleHint styleHintFromNSString(NSString *style)
{
    if ([style isEqual: @"sans-serif"])
        return QFont::SansSerif;
    else if ([style isEqual: @"monospace"])
        return QFont::Monospace;
    else if ([style isEqual: @"cursive"])
        return QFont::Cursive;
    else if ([style isEqual: @"serif"])
        return QFont::Serif;
    else if ([style isEqual: @"fantasy"])
        return QFont::Fantasy;
    else // if ([style isEqual: @"default"])
        return QFont::AnyStyle;
}

static NSInteger languageMapSort(id obj1, id obj2, void *context)
{
    NSArray *map1 = (NSArray *) obj1;
    NSArray *map2 = (NSArray *) obj2;
    NSArray *languages = (NSArray *) context;

    NSString *lang1 = [map1 objectAtIndex: 0];
    NSString *lang2 = [map2 objectAtIndex: 0];

    return [languages indexOfObject: lang1] - [languages indexOfObject: lang2];
}

QCoreTextFontDatabase::QCoreTextFontDatabase()
{
    QSettings appleSettings(QLatin1String("apple.com"));
    QVariant appleValue = appleSettings.value(QLatin1String("AppleAntiAliasingThreshold"));
    if (appleValue.isValid())
        qt_antialiasing_threshold = appleValue.toInt();
}

QCoreTextFontDatabase::~QCoreTextFontDatabase()
{
}

static QString familyNameFromPostScriptName(QHash<QString, QString> &psNameToFamily,
                                            NSString *psName)
{
    QString name = qt_mac_NSStringToQString(psName);
    if (psNameToFamily.contains(name))
        return psNameToFamily[name];
    else {
        // Some of the font name in DefaultFontFallbacks.plist are hidden fonts like AquaHiraKaku,
        // their family name begins with a dot, like ".AquaHiraKaku" or ".Apple Symbols Fallback",
        // the only way (I've found) to get it are actually creating a CTFont with them. We only
        // need to do it once though.
        QCFType<CTFontRef> font = CTFontCreateWithName((CFStringRef) psName, 12.0, NULL);
        if (font) {
            QCFString family = CTFontCopyFamilyName(font);
            psNameToFamily[name] = family;
            return family;
        }
        return name;
    }
}

void QCoreTextFontDatabase::populateFontDatabase()
{
    QCFType<CTFontCollectionRef> collection = CTFontCollectionCreateFromAvailableFonts(0);
    if (! collection)
        return;

    QCFType<CFArrayRef> fonts = CTFontCollectionCreateMatchingFontDescriptors(collection);
    if (! fonts)
        return;

    QString foundryName = QLatin1String("CoreText");
    const int numFonts = CFArrayGetCount(fonts);
    QHash<QString, QString> psNameToFamily;
    for (int i = 0; i < numFonts; ++i) {
        CTFontDescriptorRef font = (CTFontDescriptorRef) CFArrayGetValueAtIndex(fonts, i);
        QCFString familyName = (CFStringRef) CTFontDescriptorCopyLocalizedAttribute(font, kCTFontFamilyNameAttribute, NULL);
        QCFType<CFDictionaryRef> styles = (CFDictionaryRef) CTFontDescriptorCopyAttribute(font, kCTFontTraitsAttribute);
        QFont::Weight weight = QFont::Normal;
        QFont::Style style = QFont::StyleNormal;
        QFont::Stretch stretch = QFont::Unstretched;
        bool fixedPitch = false;

        if (styles) {
            if (CFNumberRef weightValue = (CFNumberRef) CFDictionaryGetValue(styles, kCTFontWeightTrait)) {
                Q_ASSERT(CFNumberIsFloatType(weightValue));
                double d;
                if (CFNumberGetValue(weightValue, kCFNumberDoubleType, &d))
                    weight = (d > 0.0) ? QFont::Bold : QFont::Normal;
            }
            if (CFNumberRef italic = (CFNumberRef) CFDictionaryGetValue(styles, kCTFontSlantTrait)) {
                Q_ASSERT(CFNumberIsFloatType(italic));
                double d;
                if (CFNumberGetValue(italic, kCFNumberDoubleType, &d)) {
                    if (d > 0.0)
                        style = QFont::StyleItalic;
                }
            }
            if (CFNumberRef symbolic = (CFNumberRef) CFDictionaryGetValue(styles, kCTFontSymbolicTrait)) {
                int d;
                if (CFNumberGetValue(symbolic, kCFNumberSInt32Type, &d)) {
                    if (d & kCTFontMonoSpaceTrait)
                        fixedPitch = true;
                    if (d & kCTFontExpandedTrait)
                        stretch = QFont::Expanded;
                    else if (d & kCTFontCondensedTrait)
                        stretch = QFont::Condensed;
                }
            }
        }

        int pixelSize = 0;
        if (QCFType<CFNumberRef> size = (CFNumberRef) CTFontDescriptorCopyAttribute(font, kCTFontSizeAttribute)) {
            if (CFNumberIsFloatType(size)) {
                double d;
                CFNumberGetValue(size, kCFNumberDoubleType, &d);
                pixelSize = d;
            } else {
                CFNumberGetValue(size, kCFNumberIntType, &pixelSize);
            }
        }

        QSupportedWritingSystems writingSystems;
        if (QCFType<CFArrayRef> languages = (CFArrayRef) CTFontDescriptorCopyAttribute(font, kCTFontLanguagesAttribute)) {
            CFIndex length = CFArrayGetCount(languages);
            for (int i = 1; i < LanguageCount; ++i) {
                if (!languageForWritingSystem[i])
                    continue;
                QCFString lang = CFStringCreateWithCString(NULL, languageForWritingSystem[i], kCFStringEncodingASCII);
                if (CFArrayContainsValue(languages, CFRangeMake(0, length), lang))
                    writingSystems.setSupported(QFontDatabase::WritingSystem(i));
            }
        }

        CFRetain(font);
        QPlatformFontDatabase::registerFont(familyName, foundryName, weight, style, stretch,
                                            true /* antialiased */, true /* scalable */,
                                            pixelSize, fixedPitch, writingSystems, (void *) font);
        CFStringRef psName = (CFStringRef) CTFontDescriptorCopyAttribute(font, kCTFontNameAttribute);
        // we need PostScript Name to family name mapping for fallback list construction
        psNameToFamily[qt_mac_NSStringToQString((NSString *) psName)] = familyName;
        CFRelease(psName);
    }

    NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
    NSArray *languages = [defaults stringArrayForKey: @"AppleLanguages"];

    NSAutoreleasePool *pool = [NSAutoreleasePool new];

    NSDictionary *fallbackDict = [NSDictionary dictionaryWithContentsOfFile: @"/System/Library/Frameworks/ApplicationServices.framework/Frameworks/CoreText.framework/Resources/DefaultFontFallbacks.plist"];

    for (NSString *style in [fallbackDict allKeys]) {
        NSArray *list = [fallbackDict valueForKey: style];
        QFont::StyleHint styleHint = styleHintFromNSString(style);
        QStringList fallbackList;
        for (id item in list) {
            // sort the array based on system language preferences
            if ([item isKindOfClass: [NSArray class]]) {
                NSArray *langs = [(NSArray *) item sortedArrayUsingFunction: languageMapSort
                                                                    context: languages];
                for (NSArray *map in langs)
                    fallbackList.append(familyNameFromPostScriptName(psNameToFamily, [map objectAtIndex: 1]));
            }
            else if ([item isKindOfClass: [NSString class]])
                fallbackList.append(familyNameFromPostScriptName(psNameToFamily, item));
        }
        fallbackLists[styleHint] = fallbackList;
    }

    [pool release];
}

void QCoreTextFontDatabase::releaseHandle(void *handle)
{
    CFRelease(CTFontDescriptorRef(handle));
}

QFontEngine *QCoreTextFontDatabase::fontEngine(const QFontDef &f, QUnicodeTables::Script script, void *usrPtr)
{
    Q_UNUSED(script);

    CTFontDescriptorRef descriptor = (CTFontDescriptorRef) usrPtr;
    CTFontRef font = CTFontCreateWithFontDescriptor(descriptor, f.pointSize, NULL);
    if (font) {
        QFontEngine *engine = new QCoreTextFontEngine(font, f);
        engine->fontDef = f;
        CFRelease(font);
        return engine;
    }

    return NULL;
}

QStringList QCoreTextFontDatabase::fallbacksForFamily(const QString family, const QFont::Style &style, const QFont::StyleHint &styleHint, const QUnicodeTables::Script &script) const
{
    Q_UNUSED(family);
    Q_UNUSED(style);
    Q_UNUSED(script);
    return fallbackLists[styleHint];
}

QStringList QCoreTextFontDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName)
{
    ATSFontContainerRef fontContainer;
    OSStatus e;

    if (!fontData.isEmpty()) {
        e = ATSFontActivateFromMemory((void *) fontData.constData(), fontData.size(),
                                      kATSFontContextLocal, kATSFontFormatUnspecified, NULL,
                                      kATSOptionFlagsDefault, &fontContainer);
    } else {
        OSErr qt_mac_create_fsref(const QString &file, FSRef *fsref);
        FSRef ref;
        if (qt_mac_create_fsref(fileName, &ref) != noErr)
            return QStringList();
        e = ATSFontActivateFromFileReference(&ref, kATSFontContextLocal, kATSFontFormatUnspecified, 0,
                                             kATSOptionFlagsDefault, &fontContainer);
    }

    if (e == noErr) {
        ItemCount fontCount = 0;
        e = ATSFontFindFromContainer(fontContainer, kATSOptionFlagsDefault, 0, 0, &fontCount);
        if (e != noErr)
            return QStringList();

        QVarLengthArray<ATSFontRef> containedFonts(fontCount);
        e = ATSFontFindFromContainer(fontContainer, kATSOptionFlagsDefault, fontCount, containedFonts.data(), &fontCount);
        if (e != noErr)
            return QStringList();

        QStringList families;
        for (int i = 0; i < containedFonts.size(); ++i) {
            QCFType<CTFontRef> font = CTFontCreateWithPlatformFont(containedFonts[i], 12.0, NULL, NULL);
            families.append(QCFString(CTFontCopyFamilyName(font)));
        }

        return families;
    }

    return QStringList();
}

QFont QCoreTextFontDatabase::defaultFont() const
{
    if (defaultFontName.isEmpty()) {
        QCFType<CTFontRef> font = CTFontCreateUIFontForLanguage(kCTFontSystemFontType, 12.0, NULL);
        defaultFontName = (QString) QCFString(CTFontCopyFullName(font));
    }

    return QFont(defaultFontName);
}

