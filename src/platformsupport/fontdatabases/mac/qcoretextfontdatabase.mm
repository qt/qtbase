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

#include "qglobal.h"

#ifdef Q_OS_MACX
#import <Cocoa/Cocoa.h>
#import <IOKit/graphics/IOGraphicsLib.h>
#endif

#include "qcoretextfontdatabase_p.h"
#include "qfontengine_coretext_p.h"
#include <QtCore/QSettings>
#include <QtGui/QGuiApplication>

QT_BEGIN_NAMESPACE

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
    "zh-Hans", // SimplifiedChinese
    "zh-Hant", // TraditionalChinese
    "ja",  // Japanese
    "ko",  // Korean
    "vi",  // Vietnamese
    0, // Symbol
    "sga", // Ogham
    "non", // Runic
    "man" // N'Ko
};
enum { LanguageCount = sizeof(languageForWritingSystem) / sizeof(const char *) };

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
#ifdef Q_OS_MACX
    QSettings appleSettings(QLatin1String("apple.com"));
    QVariant appleValue = appleSettings.value(QLatin1String("AppleAntiAliasingThreshold"));
    if (appleValue.isValid())
        QCoreTextFontEngine::antialiasingThreshold = appleValue.toInt();

    /*
        font_smoothing = 0 means no smoothing, while 1-3 means subpixel
        antialiasing with different hinting styles (but we don't care about the
        exact value, only if subpixel rendering is available or not)
    */
    int font_smoothing = 0;
    appleValue = appleSettings.value(QLatin1String("AppleFontSmoothing"));
    if (appleValue.isValid()) {
        font_smoothing = appleValue.toInt();
    } else {
        // non-Apple displays do not provide enough information about subpixel rendering so
        // draw text with cocoa and compare pixel colors to see if subpixel rendering is enabled
        int w = 10;
        int h = 10;
        NSRect rect = NSMakeRect(0.0, 0.0, w, h);
        NSImage *fontImage = [[NSImage alloc] initWithSize:NSMakeSize(w, h)];

        [fontImage lockFocus];

        [[NSColor whiteColor] setFill];
        NSRectFill(rect);

        NSString *str = @"X\\";
        NSFont *font = [NSFont fontWithName:@"Helvetica" size:10.0];
        NSMutableDictionary *attrs = [NSMutableDictionary dictionary];
        [attrs setObject:font forKey:NSFontAttributeName];
        [attrs setObject:[NSColor blackColor] forKey:NSForegroundColorAttributeName];

        [str drawInRect:rect withAttributes:attrs];

        NSBitmapImageRep *nsBitmapImage = [[NSBitmapImageRep alloc] initWithFocusedViewRect:rect];

        [fontImage unlockFocus];

        float red, green, blue;
        for (int x = 0; x < w; x++) {
            for (int y = 0; y < h; y++) {
                NSColor *pixelColor = [nsBitmapImage colorAtX:x y:y];
                red = [pixelColor redComponent];
                green = [pixelColor greenComponent];
                blue = [pixelColor blueComponent];
                if (red != green || red != blue)
                    font_smoothing = 1;
            }
        }

        [nsBitmapImage release];
        [fontImage release];
    }
    QCoreTextFontEngine::defaultGlyphFormat = (font_smoothing > 0
                                               ? QFontEngineGlyphCache::Raster_RGBMask
                                               : QFontEngineGlyphCache::Raster_A8);
#else
    QCoreTextFontEngine::defaultGlyphFormat = QFontEngineGlyphCache::Raster_A8;
#endif
}

QCoreTextFontDatabase::~QCoreTextFontDatabase()
{
}

void QCoreTextFontDatabase::populateFontDatabase()
{
    NSAutoreleasePool *pool = [[NSAutoreleasePool alloc] init];

    QCFType<CTFontCollectionRef> collection = CTFontCollectionCreateFromAvailableFonts(0);
    if (! collection)
        return;

    QCFType<CFArrayRef> fonts = CTFontCollectionCreateMatchingFontDescriptors(collection);
    if (! fonts)
        return;

    QString foundryName = QLatin1String("CoreText");
    const int numFonts = CFArrayGetCount(fonts);
    for (int i = 0; i < numFonts; ++i) {
        CTFontDescriptorRef font = (CTFontDescriptorRef) CFArrayGetValueAtIndex(fonts, i);
        QCFString familyName = (CFStringRef) CTFontDescriptorCopyLocalizedAttribute(font, kCTFontFamilyNameAttribute, NULL);
        QCFString styleName = (CFStringRef)CTFontDescriptorCopyLocalizedAttribute(font, kCTFontStyleNameAttribute, NULL);
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
        QPlatformFontDatabase::registerFont(familyName, styleName, foundryName, weight, style, stretch,
                                            true /* antialiased */, true /* scalable */,
                                            pixelSize, fixedPitch, writingSystems, (void *) font);

        // We need to map back and forth between PostScript-names and family-names for fallback list construction
        CFStringRef psName = (CFStringRef) CTFontDescriptorCopyAttribute(font, kCTFontNameAttribute);
        psNameToFamily[QCFString::toQString((NSString *) psName)] = familyName;
        familyNameToPsName[familyName] = QCFString::toQString((NSString *) psName);
        CFRelease(psName);
    }

    [pool release];
}

void QCoreTextFontDatabase::releaseHandle(void *handle)
{
    CFRelease(CTFontDescriptorRef(handle));
}

QFontEngine *QCoreTextFontDatabase::fontEngine(const QFontDef &f, QChar::Script script, void *usrPtr)
{
    Q_UNUSED(script);

    qreal scaledPointSize = f.pixelSize;

    // When 96 DPI is forced, the Mac plugin will use DPI 72 for some
    // fonts (hardcoded in qcocoaintegration.mm) and 96 for others. This
    // discrepancy makes it impossible to find the correct point size
    // here without having the DPI used for the font. Until a proper
    // solution (requiring API change) can be made, we simply fall back
    // to passing in the point size to retain old behavior.
    if (QGuiApplication::testAttribute(Qt::AA_Use96Dpi))
        scaledPointSize = f.pointSize;

    CTFontDescriptorRef descriptor = (CTFontDescriptorRef) usrPtr;
    CTFontRef font = CTFontCreateWithFontDescriptor(descriptor, scaledPointSize, NULL);
    if (font) {
        QFontEngine *engine = new QCoreTextFontEngine(font, f);
        engine->fontDef = f;
        CFRelease(font);
        return engine;
    }

    return NULL;
}

QFontEngine *QCoreTextFontDatabase::fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference)
{
    Q_UNUSED(hintingPreference);

    QCFType<CGDataProviderRef> dataProvider = CGDataProviderCreateWithData(NULL,
            fontData.constData(), fontData.size(), NULL);

    CGFontRef cgFont = CGFontCreateWithDataProvider(dataProvider);

    QFontEngine *fontEngine = NULL;
    if (cgFont == NULL) {
        qWarning("QRawFont::platformLoadFromData: CGFontCreateWithDataProvider failed");
    } else {
        QFontDef def;
        def.pixelSize = pixelSize;
        def.pointSize = pixelSize * 72.0 / qt_defaultDpi();
        fontEngine = new QCoreTextFontEngine(cgFont, def);
        CFRelease(cgFont);
    }

    return fontEngine;
}

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

static QString familyNameFromPostScriptName(QHash<QString, QString> &psNameToFamily,
                                            NSString *psName)
{
    QString name = QCFString::toQString(psName);
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

QStringList QCoreTextFontDatabase::fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const
{
    Q_UNUSED(style);
    Q_UNUSED(script);

    static QHash<QString, QStringList> fallbackLists;

#if QT_MAC_PLATFORM_SDK_EQUAL_OR_ABOVE(__MAC_10_8, __IPHONE_6_0)
  // CTFontCopyDefaultCascadeListForLanguages is available in the SDK
  #if QT_MAC_DEPLOYMENT_TARGET_BELOW(__MAC_10_8, __IPHONE_6_0)
    // But we have to feature check at runtime
    if (&CTFontCopyDefaultCascadeListForLanguages)
  #endif
    {
        if (fallbackLists.contains(family))
            return fallbackLists.value(family);

        if (!familyNameToPsName.contains(family))
            const_cast<QCoreTextFontDatabase*>(this)->populateFontDatabase();

        QCFType<CTFontRef> font = CTFontCreateWithName(QCFString(familyNameToPsName[family]), 12.0, NULL);
        if (font) {
            NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
            NSArray *languages = [defaults stringArrayForKey: @"AppleLanguages"];

            QCFType<CFArrayRef> cascadeList = (CFArrayRef) CTFontCopyDefaultCascadeListForLanguages(font, (CFArrayRef) languages);
            if (cascadeList) {
                QStringList fallbackList;
                const int numCascades = CFArrayGetCount(cascadeList);
                for (int i = 0; i < numCascades; ++i) {
                    CTFontDescriptorRef fontFallback = (CTFontDescriptorRef) CFArrayGetValueAtIndex(cascadeList, i);
                    QCFString fallbackFamilyName = (CFStringRef) CTFontDescriptorCopyLocalizedAttribute(fontFallback, kCTFontFamilyNameAttribute, NULL);
                    fallbackList.append(QCFString::toQString(fallbackFamilyName));
                }
                fallbackLists[family] = fallbackList;
            }
        }

        if (fallbackLists.contains(family))
            return fallbackLists.value(family);
    }
#else
    Q_UNUSED(family);
#endif

    // We were not able to find a fallback for the specific family,
    // so we fall back to the stylehint.

    static const QString styleLookupKey = QString::fromLatin1(".QFontStyleHint_%1");

    static bool didPopulateStyleFallbacks = false;
    if (!didPopulateStyleFallbacks) {
#if defined(Q_OS_MACX)
        // Ensure we have the psNameToFamily mapping set up
        const_cast<QCoreTextFontDatabase*>(this)->populateFontDatabase();

        NSUserDefaults *defaults = [NSUserDefaults standardUserDefaults];
        NSArray *languages = [defaults stringArrayForKey: @"AppleLanguages"];

        NSDictionary *fallbackDict = [NSDictionary dictionaryWithContentsOfFile: @"/System/Library/Frameworks/ApplicationServices.framework/Frameworks/CoreText.framework/Resources/DefaultFontFallbacks.plist"];

        for (NSString *style in [fallbackDict allKeys]) {
            NSArray *list = [fallbackDict valueForKey: style];
            QFont::StyleHint fallbackStyleHint = styleHintFromNSString(style);
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

            if (QCoreTextFontEngine::supportsColorGlyphs())
                fallbackList.append(QLatin1String("Apple Color Emoji"));

            fallbackLists[styleLookupKey.arg(fallbackStyleHint)] = fallbackList;
        }
#else
        QStringList staticFallbackList;
        staticFallbackList << QString::fromLatin1("Helvetica,Apple Color Emoji,Geeza Pro,Arial Hebrew,Thonburi,Kailasa"
            "Hiragino Kaku Gothic ProN,.Heiti J,Apple SD Gothic Neo,.Heiti K,Heiti SC,Heiti TC"
            "Bangla Sangam MN,Devanagari Sangam MN,Gujarati Sangam MN,Gurmukhi MN,Kannada Sangam MN"
            "Malayalam Sangam MN,Oriya Sangam MN,Sinhala Sangam MN,Tamil Sangam MN,Telugu Sangam MN"
            "Euphemia UCAS,.PhoneFallback").split(QLatin1String(","));

        for (int i = QFont::Helvetica; i <= QFont::Fantasy; ++i)
            fallbackLists[styleLookupKey.arg(i)] = staticFallbackList;
#endif

        didPopulateStyleFallbacks = true;
    }

    Q_ASSERT(!fallbackLists.isEmpty());
    return fallbackLists[styleLookupKey.arg(styleHint)];
}

#ifdef Q_OS_MACX
QStringList QCoreTextFontDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName)
{
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_8
    if (QSysInfo::QSysInfo::MacintoshVersion >= QSysInfo::MV_10_8) {
        CTFontRef font = NULL;

        if (!fontData.isEmpty()) {
            QCFType<CGDataProviderRef> dataProvider = CGDataProviderCreateWithData(NULL,
                fontData.constData(), fontData.size(), NULL);
            CGFontRef cgFont = CGFontCreateWithDataProvider(dataProvider);
            if (cgFont) {
                CFErrorRef error;
                bool success = CTFontManagerRegisterGraphicsFont(cgFont, &error);
                if (success) {
                    font = CTFontCreateWithGraphicsFont(cgFont, 0.0, NULL, NULL);
                } else {
                    NSLog(@"Unable to register font: %@", error);
                    CFRelease(error);
                }
                CGFontRelease(cgFont);
            }
        } else {
            CFErrorRef error;
            QCFType<CFURLRef> fontURL = CFURLCreateWithFileSystemPath(NULL, QCFString(fileName), kCFURLPOSIXPathStyle, false);
            bool success = CTFontManagerRegisterFontsForURL(fontURL, kCTFontManagerScopeProcess, &error);
            if (success) {
                const void *keys[] = { fontURL };
                const void *values[] = { kCTFontURLAttribute };
                QCFType<CFDictionaryRef> attributes = CFDictionaryCreate(NULL, keys, values, 1,
                    &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
                QCFType<CTFontDescriptorRef> descriptor = CTFontDescriptorCreateWithAttributes(attributes);
                font = CTFontCreateWithFontDescriptor(descriptor, 0.0, NULL);
            } else {
                NSLog(@"Unable to register font: %@", error);
                CFRelease(error);
            }
        }

        if (font) {
            QStringList families;
            families.append(QCFString(CTFontCopyFamilyName(font)));
            CFRelease(font);
            return families;
        }
    } else {
#else
    ATSFontContainerRef fontContainer;
    OSStatus e;

    if (!fontData.isEmpty()) {
        e = ATSFontActivateFromMemory((void *) fontData.constData(), fontData.size(),
                                      kATSFontContextLocal, kATSFontFormatUnspecified, NULL,
                                      kATSOptionFlagsDefault, &fontContainer);
    } else {
        FSRef ref;
        OSErr qt_mac_create_fsref(const QString &file, FSRef *fsref);
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
#endif
#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_8
    }
#endif

    return QStringList();
}
#endif

QFont QCoreTextFontDatabase::defaultFont() const
{
    if (defaultFontName.isEmpty()) {
        QCFType<CTFontRef> font = CTFontCreateUIFontForLanguage(kCTFontSystemFontType, 12.0, NULL);
        defaultFontName = (QString) QCFString(CTFontCopyFullName(font));
    }

    return QFont(defaultFontName);
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

QT_END_NAMESPACE

