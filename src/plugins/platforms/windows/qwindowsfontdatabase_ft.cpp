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

#include "qwindowsfontdatabase_ft.h"
#include "qwindowsfontdatabase.h"
#include "qwindowscontext.h"

#include <ft2build.h>
#include FT_TRUETYPE_TABLES_H

#include <QtCore/QDir>
#include <QtCore/QDirIterator>
#include <QtCore/QSettings>
#include <QtCore/QRegularExpression>
#include <QtGui/private/qfontengine_ft_p.h>
#include <QtGui/QGuiApplication>
#include <QtGui/QFontDatabase>

#include <wchar.h>
#ifdef Q_OS_WINCE
#include <QtCore/QFile>
#include <QtEndian>
#endif

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

extern bool localizedName(const QString &name);
extern QString getEnglishName(const QString &familyName);

#ifndef Q_OS_WINCE

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
        const QRegularExpression sizeListMatch(QStringLiteral("\\s(\\d+,)+\\d+"));
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
            const QStringList fontNames = realKey.trimmed().split(QLatin1Char('&'));
            fontKey.fontNames.reserve(fontNames.size());
            foreach (const QString &fontName, fontNames)
                fontKey.fontNames.append(fontName.trimmed());
            result.append(fontKey);
        }
    }
    return result;
}

static const FontKey *findFontKey(const QString &name, int *indexIn = Q_NULLPTR)
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
     return Q_NULLPTR;
}

#else // Q_OS_WINCE

typedef struct {
    quint16 majorVersion;
    quint16 minorVersion;
    quint16 numTables;
    quint16 searchRange;
    quint16 entrySelector;
    quint16 rangeShift;
} OFFSET_TABLE;

typedef struct {
    quint32 tag;
    quint32 checkSum;
    quint32 offset;
    quint32 length;
} TABLE_DIRECTORY;

typedef struct {
    quint16 fontSelector;
    quint16 nrCount;
    quint16 storageOffset;
} NAME_TABLE_HEADER;

typedef struct {
    quint16 platformID;
    quint16 encodingID;
    quint16 languageID;
    quint16 nameID;
    quint16 stringLength;
    quint16 stringOffset;
} NAME_RECORD;

typedef struct {
    quint32 tag;
    quint16 majorVersion;
    quint16 minorVersion;
    quint32 numFonts;
} TTC_TABLE_HEADER;

static QString fontNameFromTTFile(const QString &filename, int startPos = 0)
{
    QFile f(filename);
    QString retVal;
    qint64 bytesRead;
    qint64 bytesToRead;

    if (f.open(QIODevice::ReadOnly)) {
        f.seek(startPos);
        OFFSET_TABLE ttOffsetTable;
        bytesToRead = sizeof(OFFSET_TABLE);
        bytesRead = f.read((char*)&ttOffsetTable, bytesToRead);
        if (bytesToRead != bytesRead)
            return retVal;
        ttOffsetTable.numTables = qFromBigEndian(ttOffsetTable.numTables);
        ttOffsetTable.majorVersion = qFromBigEndian(ttOffsetTable.majorVersion);
        ttOffsetTable.minorVersion = qFromBigEndian(ttOffsetTable.minorVersion);

        if (ttOffsetTable.majorVersion != 1 || ttOffsetTable.minorVersion != 0)
            return retVal;

        TABLE_DIRECTORY tblDir;
        bool found = false;

        for (int i = 0; i < ttOffsetTable.numTables; i++) {
            bytesToRead = sizeof(TABLE_DIRECTORY);
            bytesRead = f.read((char*)&tblDir, bytesToRead);
            if (bytesToRead != bytesRead)
                return retVal;
            if (qFromBigEndian(tblDir.tag) == MAKE_TAG('n', 'a', 'm', 'e')) {
                found = true;
                tblDir.length = qFromBigEndian(tblDir.length);
                tblDir.offset = qFromBigEndian(tblDir.offset);
                break;
            }
        }

        if (found) {
            f.seek(tblDir.offset);
            NAME_TABLE_HEADER ttNTHeader;
            bytesToRead = sizeof(NAME_TABLE_HEADER);
            bytesRead = f.read((char*)&ttNTHeader, bytesToRead);
            if (bytesToRead != bytesRead)
                return retVal;
            ttNTHeader.nrCount = qFromBigEndian(ttNTHeader.nrCount);
            ttNTHeader.storageOffset = qFromBigEndian(ttNTHeader.storageOffset);
            NAME_RECORD ttRecord;
            found = false;

            for (int i = 0; i < ttNTHeader.nrCount; i++) {
                bytesToRead = sizeof(NAME_RECORD);
                bytesRead = f.read((char*)&ttRecord, bytesToRead);
                if (bytesToRead != bytesRead)
                    return retVal;
                ttRecord.nameID = qFromBigEndian(ttRecord.nameID);
                if (ttRecord.nameID == 1) {
                    ttRecord.stringLength = qFromBigEndian(ttRecord.stringLength);
                    ttRecord.stringOffset = qFromBigEndian(ttRecord.stringOffset);
                    int nPos = f.pos();
                    f.seek(tblDir.offset + ttRecord.stringOffset + ttNTHeader.storageOffset);

                    QByteArray nameByteArray = f.read(ttRecord.stringLength);
                    if (!nameByteArray.isEmpty()) {
                        if (ttRecord.encodingID == 256 || ttRecord.encodingID == 768) {
                            //This is UTF-16 in big endian
                            int stringLength = ttRecord.stringLength / 2;
                            retVal.resize(stringLength);
                            QChar *data = retVal.data();
                            const ushort *srcData = (const ushort *)nameByteArray.data();
                            for (int i = 0; i < stringLength; ++i)
                                data[i] = qFromBigEndian(srcData[i]);
                            return retVal;
                        } else if (ttRecord.encodingID == 0) {
                            //This is Latin1
                            retVal = QString::fromLatin1(nameByteArray);
                        } else {
                            qWarning("Could not retrieve Font name from file: %s", qPrintable(QDir::toNativeSeparators(filename)));
                        }
                        break;
                    }
                    f.seek(nPos);
                }
            }
        }
        f.close();
    }
    return retVal;
}

static QStringList fontNamesFromTTCFile(const QString &filename)
{
    QFile f(filename);
    QStringList retVal;
    qint64 bytesRead;
    qint64 bytesToRead;

    if (f.open(QIODevice::ReadOnly)) {
        TTC_TABLE_HEADER ttcTableHeader;
        bytesToRead = sizeof(TTC_TABLE_HEADER);
        bytesRead = f.read((char*)&ttcTableHeader, bytesToRead);
        if (bytesToRead != bytesRead)
            return retVal;
        ttcTableHeader.majorVersion = qFromBigEndian(ttcTableHeader.majorVersion);
        ttcTableHeader.minorVersion = qFromBigEndian(ttcTableHeader.minorVersion);
        ttcTableHeader.numFonts = qFromBigEndian(ttcTableHeader.numFonts);

        if (ttcTableHeader.majorVersion < 1 || ttcTableHeader.majorVersion > 2)
            return retVal;
        QVarLengthArray<quint32> offsetTable(ttcTableHeader.numFonts);
        bytesToRead = sizeof(quint32) * ttcTableHeader.numFonts;
        bytesRead = f.read((char*)offsetTable.data(), bytesToRead);
        if (bytesToRead != bytesRead)
            return retVal;
        f.close();
        for (int i = 0; i < (int)ttcTableHeader.numFonts; ++i)
            retVal << fontNameFromTTFile(filename, qFromBigEndian(offsetTable[i]));
    }
    return retVal;
}

static inline QString fontSettingsOrganization() { return QStringLiteral("Qt-Project"); }
static inline QString fontSettingsApplication()  { return QStringLiteral("Qtbase"); }
static inline QString fontSettingsGroup()        { return QStringLiteral("CEFontCache"); }

static QString findFontFile(const QString &faceName)
{
    static QHash<QString, QString> fontCache;

    if (fontCache.isEmpty()) {
        QSettings settings(QSettings::SystemScope, fontSettingsOrganization(), fontSettingsApplication());
        settings.beginGroup(fontSettingsGroup());
        foreach (const QString &fontName, settings.allKeys())
            fontCache.insert(fontName, settings.value(fontName).toString());
        settings.endGroup();
    }

    QString value = fontCache.value(faceName);

    //Fallback if we haven't cached the font yet or the font got removed/renamed iterate again over all fonts
    if (value.isEmpty() || !QFile::exists(value)) {
        QSettings settings(QSettings::SystemScope, fontSettingsOrganization(), fontSettingsApplication());
        settings.beginGroup(fontSettingsGroup());

        //empty the cache first, as it seems that it is dirty
        settings.remove(QString());

        QDirIterator it(QStringLiteral("/Windows"), QStringList() << QStringLiteral("*.ttf") << QStringLiteral("*.ttc"), QDir::Files | QDir::Hidden | QDir::System);
        const QLatin1Char lowerF('f');
        const QLatin1Char upperF('F');
        while (it.hasNext()) {
            const QString fontFile = it.next();
            QStringList fontNames;
            const QChar c = fontFile[fontFile.size() - 1];
            if (c == lowerF || c == upperF)
                fontNames << fontNameFromTTFile(fontFile);
            else
                fontNames << fontNamesFromTTCFile(fontFile);
            foreach (const QString fontName, fontNames) {
                if (fontName.isEmpty())
                    continue;
                fontCache.insert(fontName, fontFile);
                settings.setValue(fontName, fontFile);

                if (localizedName(fontName)) {
                    QString englishFontName = getEnglishName(fontName);
                    fontCache.insert(englishFontName, fontFile);
                    settings.setValue(englishFontName, fontFile);
                }
            }
        }
        settings.endGroup();
        value = fontCache.value(faceName);
    }
    return value;
}
#endif // Q_OS_WINCE

static bool addFontToDatabase(const QString &faceName,
                              const QString &fullName,
                              uchar charSet,
                              const TEXTMETRIC *textmetric,
                              const FONTSIGNATURE *signature,
                              int type,
                              bool registerAlias)
{
    // the "@family" fonts are just the same as "family". Ignore them.
    if (faceName.isEmpty() || faceName.at(0) == QLatin1Char('@') || faceName.startsWith(QLatin1String("WST_")))
        return false;

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
    if (QWindowsContext::verbose > 2) {
        QString message;
        QTextStream str(&message);
        str << __FUNCTION__ << ' ' << faceName << "::" << fullName << ' ' << charSet << " TTF=" << ttf;
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
    if (registerAlias & ttf && localizedName(faceName))
        englishName = getEnglishName(faceName);

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
#ifndef Q_OS_WINCE
    const FontKey *key = findFontKey(faceName, &index);
    if (!key) {
        key = findFontKey(fullName, &index);
        if (!key && !registerAlias && englishName.isEmpty() && localizedName(faceName))
            englishName = getEnglishName(faceName);
        if (!key && !englishName.isEmpty())
            key = findFontKey(englishName, &index);
        if (!key)
            return false;
    }
    QString value = key->fileName;
#else
    QString value = findFontFile(faceName);
#endif

    if (value.isEmpty())
        return false;

    if (!QDir::isAbsolutePath(value))
#ifndef Q_OS_WINCE
        value.prepend(QFile::decodeName(qgetenv("windir") + "\\Fonts\\"));
#else
        value.prepend(QFile::decodeName("/Windows/"));
#endif

    QPlatformFontDatabase::registerFont(faceName, QString(), foundryName, weight, style, stretch,
        antialias, scalable, size, fixed, writingSystems, createFontFile(value, index));

    // add fonts windows can generate for us:
    if (weight <= QFont::DemiBold)
        QPlatformFontDatabase::registerFont(faceName, QString(), foundryName, QFont::Bold, style, stretch,
                                            antialias, scalable, size, fixed, writingSystems, createFontFile(value, index));

    if (style != QFont::StyleItalic)
        QPlatformFontDatabase::registerFont(faceName, QString(), foundryName, weight, QFont::StyleItalic, stretch,
                                            antialias, scalable, size, fixed, writingSystems, createFontFile(value, index));

    if (weight <= QFont::DemiBold && style != QFont::StyleItalic)
        QPlatformFontDatabase::registerFont(faceName, QString(), foundryName, QFont::Bold, QFont::StyleItalic, stretch,
                                            antialias, scalable, size, fixed, writingSystems, createFontFile(value, index));

    if (!englishName.isEmpty())
        QPlatformFontDatabase::registerAliasToFontFamily(faceName, englishName);

    return true;
}

#ifdef Q_OS_WINCE
static QByteArray getFntTable(HFONT hfont, uint tag)
{
    HDC hdc = GetDC(0);
    HGDIOBJ oldFont = SelectObject(hdc, hfont);
    quint32 t = qFromBigEndian<quint32>(tag);
    QByteArray buffer;

    DWORD length = GetFontData(hdc, t, 0, NULL, 0);
    if (length != GDI_ERROR) {
        buffer.resize(length);
        GetFontData(hdc, t, 0, reinterpret_cast<uchar *>(buffer.data()), length);
    }
    SelectObject(hdc, oldFont);
    return buffer;
}
#endif

static int QT_WIN_CALLBACK storeFont(const LOGFONT *logFont, const TEXTMETRIC *textmetric,
                                     DWORD type, LPARAM)
{
    const ENUMLOGFONTEX *f = reinterpret_cast<const ENUMLOGFONTEX *>(logFont);
    const QString faceName = QString::fromWCharArray(f->elfLogFont.lfFaceName);
    const QString fullName = QString::fromWCharArray(f->elfFullName);
    const uchar charSet = f->elfLogFont.lfCharSet;

    // NEWTEXTMETRICEX (passed for TT fonts) is a NEWTEXTMETRIC, which according
    // to the documentation is identical to a TEXTMETRIC except for the last four
    // members, which we don't use anyway
    const FONTSIGNATURE *signature = Q_NULLPTR;
    if (type & TRUETYPE_FONTTYPE)
        signature = &reinterpret_cast<const NEWTEXTMETRICEX *>(textmetric)->ntmFontSig;
    addFontToDatabase(faceName, fullName, charSet, textmetric, signature, type, false);

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
    lf.lfCharSet = DEFAULT_CHARSET;
    familyName.toWCharArray(lf.lfFaceName);
    lf.lfFaceName[familyName.size()] = 0;
    lf.lfPitchAndFamily = 0;
    EnumFontFamiliesEx(dummy, &lf, storeFont, 0, 0);
    ReleaseDC(0, dummy);
}

namespace {
// Context for enumerating system fonts, records whether the default font has been
// encountered, which is normally not enumerated.
struct PopulateFamiliesContext
{
    PopulateFamiliesContext(const QString &f) : systemDefaultFont(f), seenSystemDefaultFont(false) {}

    QString systemDefaultFont;
    bool seenSystemDefaultFont;
};
} // namespace

#ifndef Q_OS_WINCE

// Delayed population of font families

static int QT_WIN_CALLBACK populateFontFamilies(const LOGFONT *logFont, const TEXTMETRIC *textmetric,
                                                DWORD, LPARAM lparam)
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
            if (!key && ttf && localizedName(faceName))
                key = findFontKey(getEnglishName(faceName));
        }
        if (key) {
            QPlatformFontDatabase::registerFontFamily(faceName);
            PopulateFamiliesContext *context = reinterpret_cast<PopulateFamiliesContext *>(lparam);
            if (!context->seenSystemDefaultFont && faceName == context->systemDefaultFont)
                context->seenSystemDefaultFont = true;

            // Register current font's english name as alias
            if (ttf && localizedName(faceName)) {
                const QString englishName = getEnglishName(faceName);
                if (!englishName.isEmpty()) {
                    QPlatformFontDatabase::registerAliasToFontFamily(faceName, englishName);
                    // Check whether the system default font name is an alias of the current font family name,
                    // as on Chinese Windows, where the system font "SimSun" is an alias to a font registered under a local name
                    if (!context->seenSystemDefaultFont && englishName == context->systemDefaultFont)
                        context->seenSystemDefaultFont = true;
                }
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
    PopulateFamiliesContext context(QWindowsFontDatabase::systemDefaultFont().family());
    EnumFontFamiliesEx(dummy, &lf, populateFontFamilies, reinterpret_cast<LPARAM>(&context), 0);
    ReleaseDC(0, dummy);
    // Work around EnumFontFamiliesEx() not listing the system font
    if (!context.seenSystemDefaultFont)
        QPlatformFontDatabase::registerFontFamily(context.systemDefaultFont);
}

#else // !Q_OS_WINCE

// Non-delayed population of fonts (Windows CE).

static int QT_WIN_CALLBACK populateFontCe(ENUMLOGFONTEX* f, NEWTEXTMETRICEX *textmetric,
                                          int type, LPARAM lparam)
{
    // the "@family" fonts are just the same as "family". Ignore them.
    const wchar_t *faceNameW = f->elfLogFont.lfFaceName;
    if (faceNameW[0] && faceNameW[0] != L'@' && wcsncmp(faceNameW, L"WST_", 4)) {
        const uchar charSet = f->elfLogFont.lfCharSet;

        FONTSIGNATURE signature;
        QByteArray table;

        if (type & TRUETYPE_FONTTYPE) {
            HFONT hfont = CreateFontIndirect(&f->elfLogFont);
            table = getFntTable(hfont, MAKE_TAG('O', 'S', '/', '2'));
            DeleteObject((HGDIOBJ)hfont);
        }

        if (table.length() >= 86) {
            // See also qfontdatabase_mac.cpp, offsets taken from OS/2 table in the TrueType spec
            uchar *tableData = reinterpret_cast<uchar *>(table.data());

            signature.fsUsb[0] = qFromBigEndian<quint32>(tableData + 42);
            signature.fsUsb[1] = qFromBigEndian<quint32>(tableData + 46);
            signature.fsUsb[2] = qFromBigEndian<quint32>(tableData + 50);
            signature.fsUsb[3] = qFromBigEndian<quint32>(tableData + 54);

            signature.fsCsb[0] = qFromBigEndian<quint32>(tableData + 78);
            signature.fsCsb[1] = qFromBigEndian<quint32>(tableData + 82);
        } else {
            memset(&signature, 0, sizeof(signature));
        }

        // NEWTEXTMETRICEX is a NEWTEXTMETRIC, which according to the documentation is
        // identical to a TEXTMETRIC except for the last four members, which we don't use
        // anyway
        const QString faceName = QString::fromWCharArray(f->elfLogFont.lfFaceName);
        if (addFontToDatabase(faceName, QString::fromWCharArray(f->elfFullName),
                              charSet, (TEXTMETRIC *)textmetric, &signature, type, true)) {
            PopulateFamiliesContext *context = reinterpret_cast<PopulateFamiliesContext *>(lparam);
            if (!context->seenSystemDefaultFont && faceName == context->systemDefaultFont)
                context->seenSystemDefaultFont = true;
        }
    }

    // keep on enumerating
    return 1;
}

void QWindowsFontDatabaseFT::populateFontDatabase()
{
    LOGFONT lf;
    lf.lfCharSet = DEFAULT_CHARSET;
    HDC dummy = GetDC(0);
    lf.lfFaceName[0] = 0;
    lf.lfPitchAndFamily = 0;
    PopulateFamiliesContext context(QWindowsFontDatabase::systemDefaultFont().family());
    EnumFontFamiliesEx(dummy, &lf, (FONTENUMPROC)populateFontCe, reinterpret_cast<LPARAM>(&context), 0);
    ReleaseDC(0, dummy);
    // Work around EnumFontFamiliesEx() not listing the system font, see below.
    if (!context.seenSystemDefaultFont)
         populateFamily(context.systemDefaultFont);
}
#endif // Q_OS_WINCE

QFontEngine * QWindowsFontDatabaseFT::fontEngine(const QFontDef &fontDef, void *handle)
{
    QFontEngine *fe = QBasicFontDatabase::fontEngine(fontDef, handle);
    qCDebug(lcQpaFonts) << __FUNCTION__ << "FONTDEF" << fontDef.family << fe << handle;
    return fe;
}

QFontEngine *QWindowsFontDatabaseFT::fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference)
{
    QFontEngine *fe = QBasicFontDatabase::fontEngine(fontData, pixelSize, hintingPreference);
    qCDebug(lcQpaFonts) << __FUNCTION__ << "FONTDATA" << fontData << pixelSize << hintingPreference << fe;
    return fe;
}

QStringList QWindowsFontDatabaseFT::fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const
{
    QStringList result;

    result.append(QWindowsFontDatabase::familyForStyleHint(styleHint));

#ifdef Q_OS_WINCE
    QSettings settings(QLatin1String("HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\FontLink\\SystemLink"), QSettings::NativeFormat);
    const QStringList fontList = settings.value(family).toStringList();
    foreach (const QString &fallback, fontList) {
        const int sep = fallback.indexOf(QLatin1Char(','));
        if (sep > 0)
            result << fallback.mid(sep + 1);
    }
#endif

    result.append(QWindowsFontDatabase::extraTryFontsForFamily(family));

    result.append(QBasicFontDatabase::fallbacksForFamily(family, style, styleHint, script));

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
