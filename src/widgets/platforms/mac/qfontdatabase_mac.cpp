/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#include <private/qt_mac_p.h>
#include "qfontengine_p.h"
#include <qfile.h>
#include <qabstractfileengine.h>
#include <stdlib.h>
#include <qendian.h>
#include <private/qfontengine_coretext_p.h>
#include <private/qfontengine_mac_p.h>

QT_BEGIN_NAMESPACE

int qt_mac_pixelsize(const QFontDef &def, int dpi); //qfont_mac.cpp
int qt_mac_pointsize(const QFontDef &def, int dpi); //qfont_mac.cpp


static void initializeDb()
{
    QFontDatabasePrivate *db = privateDb();
    if(!db || db->count)
        return;

#if MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5
if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5) {
    QCFType<CTFontCollectionRef> collection = CTFontCollectionCreateFromAvailableFonts(0);
    if(!collection)
        return;
    QCFType<CFArrayRef> fonts = CTFontCollectionCreateMatchingFontDescriptors(collection);
    if(!fonts)
        return;
    QString foundry_name = "CoreText";
    const int numFonts = CFArrayGetCount(fonts);
    for(int i = 0; i < numFonts; ++i) {
        CTFontDescriptorRef font = (CTFontDescriptorRef)CFArrayGetValueAtIndex(fonts, i);

        QCFString family_name = (CFStringRef)CTFontDescriptorCopyAttribute(font, kCTFontFamilyNameAttribute);
        QCFString style_name = (CFStringRef)CTFontDescriptorCopyAttribute(font, kCTFontStyleNameAttribute);
        QtFontFamily *family = db->family(family_name, true);
        for(int ws = 1; ws < QFontDatabase::WritingSystemsCount; ++ws)
            family->writingSystems[ws] = QtFontFamily::Supported;
        QtFontFoundry *foundry = family->foundry(foundry_name, true);

        QtFontStyle::Key styleKey;
        QString styleName = style_name;
        if(QCFType<CFDictionaryRef> styles = (CFDictionaryRef)CTFontDescriptorCopyAttribute(font, kCTFontTraitsAttribute)) {
            if(CFNumberRef weight = (CFNumberRef)CFDictionaryGetValue(styles, kCTFontWeightTrait)) {
                Q_ASSERT(CFNumberIsFloatType(weight));
                double d;
                if(CFNumberGetValue(weight, kCFNumberDoubleType, &d)) {
                    //qDebug() << "BOLD" << (QString)family_name << d;
                    styleKey.weight = (d > 0.0) ? QFont::Bold : QFont::Normal;
                }
            }
            if(CFNumberRef italic = (CFNumberRef)CFDictionaryGetValue(styles, kCTFontSlantTrait)) {
                Q_ASSERT(CFNumberIsFloatType(italic));
                double d;
                if(CFNumberGetValue(italic, kCFNumberDoubleType, &d)) {
                    //qDebug() << "ITALIC" << (QString)family_name << d;
                    if (d > 0.0)
                        styleKey.style = QFont::StyleItalic;
                }
            }
        }

        QtFontStyle *style = foundry->style(styleKey, styleName, true);
        style->smoothScalable = true;
        if(QCFType<CFNumberRef> size = (CFNumberRef)CTFontDescriptorCopyAttribute(font, kCTFontSizeAttribute)) {
            //qDebug() << "WHEE";
            int pixel_size=0;
            if(CFNumberIsFloatType(size)) {
                double d;
                CFNumberGetValue(size, kCFNumberDoubleType, &d);
                pixel_size = d;
            } else {
                CFNumberGetValue(size, kCFNumberIntType, &pixel_size);
            }
            //qDebug() << "SIZE" << (QString)family_name << pixel_size;
            if(pixel_size)
                style->pixelSize(pixel_size, true);
        } else {
            //qDebug() << "WTF?";
        }
    }
} else 
#endif
    {
    }

}

static inline void load(const QString & = QString(), int = -1)
{
    initializeDb();
}

static const char *styleHint(const QFontDef &request)
{
    const char *stylehint = 0;
    switch (request.styleHint) {
    case QFont::SansSerif:
        stylehint = "Arial";
        break;
    case QFont::Serif:
        stylehint = "Times New Roman";
        break;
    case QFont::TypeWriter:
        stylehint = "Courier New";
        break;
    default:
        if (request.fixedPitch)
            stylehint = "Courier New";
        break;
    }
    return stylehint;
}

static inline float weightToFloat(unsigned int weight)
{
    return (weight - 50) / 100.0;
}

static QFontEngine *loadFromDatabase(const QFontDef &req, const QFontPrivate *d)
{
    QCFString fontName = NULL;

    QStringList family_list = familyList(req);

    const char *stylehint = styleHint(req);
    if (stylehint)
        family_list << QLatin1String(stylehint);

    // add QFont::defaultFamily() to the list, for compatibility with previous versions
    family_list << QApplication::font().defaultFamily();

    QMutexLocker locker(fontDatabaseMutex());
    QFontDatabasePrivate *db = privateDb();
    if (!db->count)
        initializeDb();
    for (int i = 0; i < family_list.size(); ++i) {
        for (int k = 0; k < db->count; ++k) {
            if (db->families[k]->name.compare(family_list.at(i), Qt::CaseInsensitive) == 0) {
                QByteArray family_name = db->families[k]->name.toUtf8();
                QCFType<CTFontRef> ctFont = CTFontCreateWithName(QCFString(db->families[k]->name), 12, NULL);
                if (ctFont) {
                    fontName = CTFontCopyFullName(ctFont);
                    goto found;
                }
            }
        }
    }
found:
    if (fontName)
        return new QCoreTextFontEngineMulti(fontName, req, d->kerning);
    return NULL;
}

void QFontDatabase::load(const QFontPrivate *d, int script)
{
    // sanity checks
    if(!qApp)
        qWarning("QFont: Must construct a QApplication before a QFont");

    Q_ASSERT(script >= 0 && script < QUnicodeTables::ScriptCount);
    Q_UNUSED(script);

    QFontDef req = d->request;
    req.pixelSize = qt_mac_pixelsize(req, d->dpi);

    // set the point size to 0 to get better caching
    req.pointSize = 0;
    QFontCache::Key key = QFontCache::Key(req, QUnicodeTables::Common, d->screen);

    if(!(d->engineData = QFontCache::instance()->findEngineData(key))) {
        d->engineData = new QFontEngineData;
        QFontCache::instance()->insertEngineData(key, d->engineData);
    } else {
        d->engineData->ref.ref();
    }
    if(d->engineData->engine) // already loaded
        return;

    // set it to the actual pointsize, so QFontInfo will do the right thing
    req.pointSize = qt_mac_pointsize(d->request, d->dpi);

    QFontEngine *e = QFontCache::instance()->findEngine(key);
    if(!e && qt_enable_test_font && req.family == QLatin1String("__Qt__Box__Engine__")) {
        e = new QTestFontEngine(req.pixelSize);
        e->fontDef = req;
    }

    if(e) {
        e->ref.ref();
        d->engineData->engine = e;
        return; // the font info and fontdef should already be filled
    }

    QFontEngine *engine = NULL;
    // Shortcut to get the font directly without going through the font database
    if (!req.family.isEmpty() && !req.styleName.isEmpty()) {
        QCFString expectedFamily = QCFString(req.family);
        QCFString expectedStyle = QCFString(req.styleName);

        QCFType<CFMutableDictionaryRef> attributes = CFDictionaryCreateMutable(NULL, 0,
            &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
        CFDictionaryAddValue(attributes, kCTFontFamilyNameAttribute, expectedFamily);
        CFDictionaryAddValue(attributes, kCTFontStyleNameAttribute, expectedStyle);

        QCFType<CTFontDescriptorRef> descriptor = CTFontDescriptorCreateWithAttributes(attributes);
        CGAffineTransform transform = qt_transform_from_fontdef(req);
        QCFType<CTFontRef> ctFont = CTFontCreateWithFontDescriptor(descriptor, req.pixelSize, &transform);
        if (ctFont) {
            QCFString familyName = CTFontCopyFamilyName(ctFont);
            // Only accept the font if the family name is exactly the same as we specified
            if (CFEqual(expectedFamily, familyName)) {
                engine = new QCoreTextFontEngineMulti(ctFont, req, d->kerning);
            }
        }
    }
    if (!engine)
        engine = loadFromDatabase(req, d);

    if (engine) {
        d->engineData->engine = engine;
        engine->ref.ref();
        QFontCache::instance()->insertEngine(key, engine);
    }
}

static void registerFont(QFontDatabasePrivate::ApplicationFont *fnt)
{
    ATSFontContainerRef handle;
    OSStatus e  = noErr;

    if(fnt->data.isEmpty()) {
#if (MAC_OS_X_VERSION_MAX_ALLOWED >= MAC_OS_X_VERSION_10_5)
        if (QSysInfo::MacintoshVersion >= QSysInfo::MV_10_5) {
                extern OSErr qt_mac_create_fsref(const QString &, FSRef *); // qglobal.cpp
                FSRef ref;
                if(qt_mac_create_fsref(fnt->fileName, &ref) != noErr)
                    return;

                ATSFontActivateFromFileReference(&ref, kATSFontContextLocal, kATSFontFormatUnspecified, 0, kATSOptionFlagsDefault, &handle);
        } else 
#endif
        {
#ifndef Q_WS_MAC64
                extern Q_CORE_EXPORT OSErr qt_mac_create_fsspec(const QString &, FSSpec *); // global.cpp
                FSSpec spec;
                if(qt_mac_create_fsspec(fnt->fileName, &spec) != noErr)
                    return;

                e = ATSFontActivateFromFileSpecification(&spec, kATSFontContextLocal, kATSFontFormatUnspecified,
                                                   0, kATSOptionFlagsDefault, &handle);
#endif
        }
    } else {
        e = ATSFontActivateFromMemory((void *)fnt->data.constData(), fnt->data.size(), kATSFontContextLocal,
                                           kATSFontFormatUnspecified, 0, kATSOptionFlagsDefault, &handle);

        fnt->data = QByteArray();
    }

    if(e != noErr)
        return;

    ItemCount fontCount = 0;
    e = ATSFontFindFromContainer(handle, kATSOptionFlagsDefault, 0, 0, &fontCount);
    if(e != noErr)
        return;

    QVarLengthArray<ATSFontRef> containedFonts(fontCount);
    e = ATSFontFindFromContainer(handle, kATSOptionFlagsDefault, fontCount, containedFonts.data(), &fontCount);
    if(e != noErr)
        return;

    fnt->families.clear();
    // Make sure that the family name set on the font matches what
    // kCTFontFamilyNameAttribute returns in initializeDb().
    // So far the best solution seems find the installed font
    // using CoreText and get the family name from it.
    // (ATSFontFamilyGetName appears to be the correct API, but also
    // returns the font display name.)
    for(int i = 0; i < containedFonts.size(); ++i) {
        QCFString fontPostScriptName;
        ATSFontGetPostScriptName(containedFonts[i], kATSOptionFlagsDefault, &fontPostScriptName);
        QCFType<CTFontDescriptorRef> font = CTFontDescriptorCreateWithNameAndSize(fontPostScriptName, 14);
        QCFString familyName = (CFStringRef)CTFontDescriptorCopyAttribute(font, kCTFontFamilyNameAttribute);
        fnt->families.append(familyName);
    }

    fnt->handle = handle;
}

bool QFontDatabase::removeApplicationFont(int handle)
{
    QMutexLocker locker(fontDatabaseMutex());

    QFontDatabasePrivate *db = privateDb();
    if(handle < 0 || handle >= db->applicationFonts.count())
        return false;

    OSStatus e = ATSFontDeactivate(db->applicationFonts.at(handle).handle,
                                   /*iRefCon=*/0, kATSOptionFlagsDefault);
    if(e != noErr)
        return false;

    db->applicationFonts[handle] = QFontDatabasePrivate::ApplicationFont();

    db->invalidate();
    return true;
}

bool QFontDatabase::removeAllApplicationFonts()
{
    QMutexLocker locker(fontDatabaseMutex());

    QFontDatabasePrivate *db = privateDb();
    for(int i = 0; i < db->applicationFonts.count(); ++i) {
        if(!removeApplicationFont(i))
            return false;
    }
    return true;
}

bool QFontDatabase::supportsThreadedFontRendering()
{
    return true;
}

QT_END_NAMESPACE
