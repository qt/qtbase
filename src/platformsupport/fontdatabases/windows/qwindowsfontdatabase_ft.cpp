/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include "qwindowsfontdatabase_ft_p.h"
#include "qwindowsfontdatabase_p.h"

#include <QtFontDatabaseSupport/private/qfontengine_ft_p.h>

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QSettings>
#include <QtCore/QRegularExpression>
#include <QtGui/QGuiApplication>
#include <QtGui/QFontDatabase>

#include <wchar.h>

QT_BEGIN_NAMESPACE

static inline QFontDatabase::WritingSystem writingSystemFromCharSet(uchar charSet)
{
    switch (charSet) {
    case ANSI_CHARSET:
    case EASTEUROPE_CHARSET:
    case BALTIC_CHARSET:
    case TURKISH_CHARSET:
        return QFontDatabase::Latin;
    case GREEK_CHARSET:
        return QFontDatabase::Greek;
    case RUSSIAN_CHARSET:
        return QFontDatabase::Cyrillic;
    case HEBREW_CHARSET:
        return QFontDatabase::Hebrew;
    case ARABIC_CHARSET:
        return QFontDatabase::Arabic;
    case THAI_CHARSET:
        return QFontDatabase::Thai;
    case GB2312_CHARSET:
        return QFontDatabase::SimplifiedChinese;
    case CHINESEBIG5_CHARSET:
        return QFontDatabase::TraditionalChinese;
    case SHIFTJIS_CHARSET:
        return QFontDatabase::Japanese;
    case HANGUL_CHARSET:
    case JOHAB_CHARSET:
        return QFontDatabase::Korean;
    case VIETNAMESE_CHARSET:
        return QFontDatabase::Vietnamese;
    case SYMBOL_CHARSET:
        return QFontDatabase::Symbol;
    default:
        break;
    }
    return QFontDatabase::Any;
}

static FontFile * createFontFile(const QString &fileName, int index)
{
    FontFile *fontFile = new FontFile;
    fontFile->fileName = fileName;
    fontFile->indexValue = index;
    return fontFile;
}

namespace {
struct FontKey
{
    QString fileName;
    QStringList fontNames;
};
} // namespace

typedef QVector<FontKey> FontKeys;

static FontKeys &fontKeys()
{
    static FontKeys result;
    if (result.isEmpty()) {
        const QSettings fontRegistry(QStringLiteral("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\Fonts"),
                                     QSettings::NativeFormat);
        const QStringList allKeys = fontRegistry.allKeys();
        const QString trueType = QStringLiteral("(TrueType)");
#if QT_CONFIG(regularexpression)
        const QRegularExpression sizeListMatch(QStringLiteral("\\s(\\d+,)+\\d+"));
#else
        const QRegExp sizeListMatch(QLatin1String("\\s(\\d+,)+\\d+"));
#endif
        Q_ASSERT(sizeListMatch.isValid());
        const int size = allKeys.size();
        result.reserve(size);
        for (int i = 0; i < size; ++i) {
            FontKey fontKey;
            const QString &registryFontKey = allKeys.at(i);
            fontKey.fileName = fontRegistry.value(registryFontKey).toString();
            QString realKey = registryFontKey;
            realKey.remove(trueType);
            realKey.remove(sizeListMatch);
            const auto fontNames = QStringRef(&realKey).trimmed().split(QLatin1Char('&'));
            fontKey.fontNames.reserve(fontNames.size());
            for (const QStringRef &fontName : fontNames)
                fontKey.fontNames.append(fontName.trimmed().toString());
            result.append(fontKey);
        }
    }
    return result;
}

static const FontKey *findFontKey(const QString &name, int *indexIn = nullptr)
{
     const FontKeys &keys = fontKeys();
     for (auto it = keys.constBegin(), cend = keys.constEnd(); it != cend; ++it) {
         const int index = it->fontNames.indexOf(name);
         if (index >= 0) {
             if (indexIn)
                 *indexIn = index;
             return &(*it);
         }
     }
     if (indexIn)
         *indexIn = -1;
     return nullptr;
}

static bool addFontToDatabase(QString familyName,
                              QString styleName,
                              const QString &fullName,
                              const LOGFONT &logFont,
                              const TEXTMETRIC *textmetric,
                              const FONTSIGNATURE *signature,
                              int type)
{
    // the "@family" fonts are just the same as "family". Ignore them.
    if (familyName.isEmpty() || familyName.at(0) == QLatin1Char('@') || familyName.startsWith(QLatin1String("WST_")))
        return false;

    uchar charSet = logFont.lfCharSet;

    static const int SMOOTH_SCALABLE = 0xffff;
    const QString foundryName; // No such concept.
    const bool fixed = !(textmetric->tmPitchAndFamily & TMPF_FIXED_PITCH);
    const bool ttf = (textmetric->tmPitchAndFamily & TMPF_TRUETYPE);
    const bool scalable = textmetric->tmPitchAndFamily & (TMPF_VECTOR|TMPF_TRUETYPE);
    const int size = scalable ? SMOOTH_SCALABLE : textmetric->tmHeight;
    const QFont::Style style = textmetric->tmItalic ? QFont::StyleItalic : QFont::StyleNormal;
    const bool antialias = false;
    const QFont::Weight weight = QPlatformFontDatabase::weightFromInteger(textmetric->tmWeight);
    const QFont::Stretch stretch = QFont::Unstretched;

#ifndef QT_NO_DEBUG_STREAM
    if (lcQpaFonts().isDebugEnabled()) {
        QString message;
        QTextStream str(&message);
        str << __FUNCTION__ << ' ' << familyName << "::" << fullName << ' ' << charSet << " TTF=" << ttf;
        if (type & DEVICE_FONTTYPE)
            str << " DEVICE";
        if (type & RASTER_FONTTYPE)
            str << " RASTER";
        if (type & TRUETYPE_FONTTYPE)
            str << " TRUETYPE";
        str << " scalable=" << scalable << " Size=" << size
                << " Style=" << style << " Weight=" << weight
                << " stretch=" << stretch;
        qCDebug(lcQpaFonts) << message;
    }
#endif

    QString englishName;
    QString faceName = familyName;

    QString subFamilyName;
    QString subFamilyStyle;
    if (ttf) {
        // Look-up names registered in the font
        FontNames canonicalNames = qt_getCanonicalFontNames(logFont);
        if (qt_localizedName(familyName) && !canonicalNames.name.isEmpty())
            englishName = canonicalNames.name;
        if (!canonicalNames.preferredName.isEmpty()) {
            subFamilyName = familyName;
            subFamilyStyle = styleName;
            familyName = canonicalNames.preferredName;
            styleName = canonicalNames.preferredStyle;
        }
    }

    QSupportedWritingSystems writingSystems;
    if (type & TRUETYPE_FONTTYPE) {
        Q_ASSERT(signature);
        quint32 unicodeRange[4] = {
            signature->fsUsb[0], signature->fsUsb[1],
            signature->fsUsb[2], signature->fsUsb[3]
        };
        quint32 codePageRange[2] = {
            signature->fsCsb[0], signature->fsCsb[1]
        };
        writingSystems = QPlatformFontDatabase::writingSystemsFromTrueTypeBits(unicodeRange, codePageRange);
        // ### Hack to work around problem with Thai text on Windows 7. Segoe UI contains
        // the symbol for Baht, and Windows thus reports that it supports the Thai script.
        // Since it's the default UI font on this platform, most widgets will be unable to
        // display Thai text by default. As a temporary work around, we special case Segoe UI
        // and remove the Thai script from its list of supported writing systems.
        if (writingSystems.supported(QFontDatabase::Thai) &&
                faceName == QLatin1String("Segoe UI"))
            writingSystems.setSupported(QFontDatabase::Thai, false);
    } else {
        const QFontDatabase::WritingSystem ws = writingSystemFromCharSet(charSet);
        if (ws != QFontDatabase::Any)
            writingSystems.setSupported(ws);
    }

    int index = 0;
    const FontKey *key = findFontKey(fullName, &index);
    if (!key) {
        // On non-English locales, the styles of the font may be localized in enumeration, but
        // not in the registry.
        QLocale systemLocale = QLocale::system();
        if (systemLocale.language() != QLocale::C
                && systemLocale.language() != QLocale::English
                && styleName != QLatin1String("Italic")
                && styleName != QLatin1String("Bold")) {
            key = findFontKey(qt_getEnglishName(fullName, true), &index);
        }
        if (!key)
            key = findFontKey(faceName, &index);
        if (!key && !englishName.isEmpty())
            key = findFontKey(englishName, &index);
        if (!key)
            return false;
    }
    QString value = key->fileName;
    if (value.isEmpty())
        return false;

    if (!QDir::isAbsolutePath(value))
        value.prepend(QFile::decodeName(qgetenv("windir") + "\\Fonts\\"));

    QPlatformFontDatabase::registerFont(familyName, styleName, foundryName, weight, style, stretch,
        antialias, scalable, size, fixed, writingSystems, createFontFile(value, index));

    // add fonts windows can generate for us:
    if (weight <= QFont::DemiBold && styleName.isEmpty())
        QPlatformFontDatabase::registerFont(familyName, QString(), foundryName, QFont::Bold, style, stretch,
                                            antialias, scalable, size, fixed, writingSystems, createFontFile(value, index));

    if (style != QFont::StyleItalic && styleName.isEmpty())
        QPlatformFontDatabase::registerFont(familyName, QString(), foundryName, weight, QFont::StyleItalic, stretch,
                                            antialias, scalable, size, fixed, writingSystems, createFontFile(value, index));

    if (weight <= QFont::DemiBold && style != QFont::StyleItalic && styleName.isEmpty())
        QPlatformFontDatabase::registerFont(familyName, QString(), foundryName, QFont::Bold, QFont::StyleItalic, stretch,
                                            antialias, scalable, size, fixed, writingSystems, createFontFile(value, index));

    if (!subFamilyName.isEmpty() && familyName != subFamilyName) {
        QPlatformFontDatabase::registerFont(subFamilyName, subFamilyStyle, foundryName, weight,
                                            style, stretch, antialias, scalable, size, fixed, writingSystems, createFontFile(value, index));
    }

    if (!englishName.isEmpty() && englishName != familyName)
        QPlatformFontDatabase::registerAliasToFontFamily(familyName, englishName);

    return true;
}

static int QT_WIN_CALLBACK storeFont(const LOGFONT *logFont, const TEXTMETRIC *textmetric,
                                     DWORD type, LPARAM)
{
    const ENUMLOGFONTEX *f = reinterpret_cast<const ENUMLOGFONTEX *>(logFont);
    const QString faceName = QString::fromWCharArray(f->elfLogFont.lfFaceName);
    const QString styleName = QString::fromWCharArray(f->elfStyle);
    const QString fullName = QString::fromWCharArray(f->elfFullName);

    // NEWTEXTMETRICEX (passed for TT fonts) is a NEWTEXTMETRIC, which according
    // to the documentation is identical to a TEXTMETRIC except for the last four
    // members, which we don't use anyway
    const FONTSIGNATURE *signature = nullptr;
    if (type & TRUETYPE_FONTTYPE)
        signature = &reinterpret_cast<const NEWTEXTMETRICEX *>(textmetric)->ntmFontSig;
    addFontToDatabase(faceName, styleName, fullName, *logFont, textmetric, signature, type);

    // keep on enumerating
    return 1;
}

/*!
    \brief Populate font database using EnumFontFamiliesEx().

    Normally, leaving the name empty should enumerate
    all fonts, however, system fonts like "MS Shell Dlg 2"
    are only found when specifying the name explicitly.
*/

void QWindowsFontDatabaseFT::populateFamily(const QString &familyName)
{
    qCDebug(lcQpaFonts) << familyName;
    if (familyName.size() >= LF_FACESIZE) {
        qCWarning(lcQpaFonts) << "Unable to enumerate family '" << familyName << '\'';
        return;
    }
    HDC dummy = GetDC(0);
    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));
    familyName.toWCharArray(lf.lfFaceName);
    lf.lfFaceName[familyName.size()] = 0;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfPitchAndFamily = 0;
    EnumFontFamiliesEx(dummy, &lf, storeFont, 0, 0);
    ReleaseDC(0, dummy);
}

// Delayed population of font families

static int QT_WIN_CALLBACK populateFontFamilies(const LOGFONT *logFont, const TEXTMETRIC *textmetric,
                                                DWORD, LPARAM)
{
    const ENUMLOGFONTEX *f = reinterpret_cast<const ENUMLOGFONTEX *>(logFont);
    // the "@family" fonts are just the same as "family". Ignore them.
    const wchar_t *faceNameW = f->elfLogFont.lfFaceName;
    if (faceNameW[0] && faceNameW[0] != L'@' && wcsncmp(faceNameW, L"WST_", 4)) {
        // Register only font families for which a font file exists for delayed population
        const bool ttf = textmetric->tmPitchAndFamily & TMPF_TRUETYPE;
        const QString faceName = QString::fromWCharArray(faceNameW);
        const FontKey *key = findFontKey(faceName);
        if (!key) {
            key = findFontKey(QString::fromWCharArray(f->elfFullName));
            if (!key && ttf && qt_localizedName(faceName))
                key = findFontKey(qt_getEnglishName(faceName));
        }
        if (key) {
            QPlatformFontDatabase::registerFontFamily(faceName);
            // Register current font's english name as alias
            if (ttf && qt_localizedName(faceName)) {
                const QString englishName = qt_getEnglishName(faceName);
                if (!englishName.isEmpty())
                    QPlatformFontDatabase::registerAliasToFontFamily(faceName, englishName);
            }
        }
    }
    return 1; // continue
}

void QWindowsFontDatabaseFT::populateFontDatabase()
{
    HDC dummy = GetDC(0);
    LOGFONT lf;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfFaceName[0] = 0;
    lf.lfPitchAndFamily = 0;
    EnumFontFamiliesEx(dummy, &lf, populateFontFamilies, 0, 0);
    ReleaseDC(0, dummy);
    // Work around EnumFontFamiliesEx() not listing the system font
    QString systemDefaultFamily = QWindowsFontDatabase::systemDefaultFont().family();
    if (QPlatformFontDatabase::resolveFontFamilyAlias(systemDefaultFamily) == systemDefaultFamily)
        QPlatformFontDatabase::registerFontFamily(systemDefaultFamily);
}

QFontEngine * QWindowsFontDatabaseFT::fontEngine(const QFontDef &fontDef, void *handle)
{
    QFontEngine *fe = QFreeTypeFontDatabase::fontEngine(fontDef, handle);
    qCDebug(lcQpaFonts) << __FUNCTION__ << "FONTDEF" << fontDef.family << fe << handle;
    return fe;
}

QFontEngine *QWindowsFontDatabaseFT::fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference)
{
    QFontEngine *fe = QFreeTypeFontDatabase::fontEngine(fontData, pixelSize, hintingPreference);
    qCDebug(lcQpaFonts) << __FUNCTION__ << "FONTDATA" << fontData << pixelSize << hintingPreference << fe;
    return fe;
}

QStringList QWindowsFontDatabaseFT::fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const
{
    QStringList result;
    result.append(QWindowsFontDatabase::familyForStyleHint(styleHint));
    result.append(QWindowsFontDatabase::extraTryFontsForFamily(family));
    result.append(QFreeTypeFontDatabase::fallbacksForFamily(family, style, styleHint, script));

    qCDebug(lcQpaFonts) << __FUNCTION__ << family << style << styleHint
        << script << result;

    return result;
}
QString QWindowsFontDatabaseFT::fontDir() const
{
    const QString result = QLatin1String(qgetenv("windir")) + QLatin1String("/Fonts");//QPlatformFontDatabase::fontDir();
    qCDebug(lcQpaFonts) << __FUNCTION__ << result;
    return result;
}

QFont QWindowsFontDatabaseFT::defaultFont() const
{
    return QWindowsFontDatabase::systemDefaultFont();
}

QT_END_NAMESPACE
