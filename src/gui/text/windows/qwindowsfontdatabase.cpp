// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:data-parser

#include "qwindowsfontdatabase_p.h"
#ifndef QT_NO_FREETYPE
#  include "qwindowsfontdatabase_ft_p.h" // for default font
#endif
#include "qwindowsfontengine_p.h"
#include <QtCore/qt_windows.h>

#include <QtGui/QFont>
#include <QtGui/QGuiApplication>
#include <QtGui/private/qtgui-config_p.h>

#include <QtCore/qmath.h>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QtEndian>
#include <QtCore/QStandardPaths>
#include <QtCore/private/qduplicatetracker_p.h>
#include <QtCore/private/qwinregistry_p.h>

#include <wchar.h>

#if QT_CONFIG(directwrite)
#  if QT_CONFIG(directwrite3)
#    include "qwindowsdirectwritefontdatabase_p.h"
#  endif
#  include <dwrite_2.h>
#  include <d2d1.h>
#  include "qwindowsfontenginedirectwrite_p.h"
#endif

#include <mutex>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if QT_CONFIG(directwrite)
static inline bool useDirectWrite(QFont::HintingPreference hintingPreference,
                                  const QString &familyName = QString(),
                                  bool isColorFont = false)
{
    const unsigned options = QWindowsFontDatabase::fontOptions();
    if (Q_UNLIKELY(options & QWindowsFontDatabase::DontUseDirectWriteFonts))
        return false;

    // At some scales, GDI will misrender the MingLiU font, so we force use of
    // DirectWrite to work around the issue.
    if (Q_UNLIKELY(familyName.startsWith("MingLiU"_L1)))
        return true;

    if (isColorFont)
        return (options & QWindowsFontDatabase::DontUseColorFonts) == 0;

    return hintingPreference == QFont::PreferNoHinting
        || hintingPreference == QFont::PreferVerticalHinting
        || (!qFuzzyCompare(qApp->devicePixelRatio(), 1.0) && hintingPreference == QFont::PreferDefaultHinting);
}
#endif // !QT_NO_DIRECTWRITE

/*!
    \class QWindowsFontEngineData
    \brief Static constant data shared by the font engines.
    \internal
*/

QWindowsFontEngineData::QWindowsFontEngineData()
    : fontSmoothingGamma(QWindowsFontDatabase::fontSmoothingGamma())
{
    // from qapplication_win.cpp
    UINT result = 0;
    if (SystemParametersInfo(SPI_GETFONTSMOOTHINGTYPE, 0, &result, 0))
        clearTypeEnabled = (result == FE_FONTSMOOTHINGCLEARTYPE);

    const qreal gray_gamma = 2.31;
    for (int i=0; i<256; ++i)
        pow_gamma[i] = uint(qRound(qPow(i / qreal(255.), gray_gamma) * 2047));

    HDC displayDC = GetDC(0);
    hdc = CreateCompatibleDC(displayDC);
    ReleaseDC(0, displayDC);
}

unsigned QWindowsFontDatabase::m_fontOptions = 0;

void QWindowsFontDatabase::setFontOptions(unsigned options)
{
    m_fontOptions = options & (QWindowsFontDatabase::DontUseDirectWriteFonts |
                               QWindowsFontDatabase::DontUseColorFonts);
}

unsigned QWindowsFontDatabase::fontOptions()
{
    return m_fontOptions;
}

qreal QWindowsFontDatabase::fontSmoothingGamma()
{
    int winSmooth;
    qreal result = 1;
    if (SystemParametersInfo(0x200C /* SPI_GETFONTSMOOTHINGCONTRAST */, 0, &winSmooth, 0))
        result = qreal(winSmooth) / qreal(1000.0);

    // Safeguard ourselves against corrupt registry values...
    if (result > 5 || result < 1)
        result = qreal(1.4);
    return result;
}

/*!
    \class QWindowsFontDatabase
    \brief Font database for Windows

    \note The Qt 4.8 WIndows font database employed a mechanism of
    delayed population of the database again passing a font name
    to EnumFontFamiliesEx(), working around the fact that
    EnumFontFamiliesEx() does not list all fonts by default.
    This should be introduced to QPA as well?

    \internal
*/

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug d, const QFontDef &def)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    d << "QFontDef(Family=\"" << def.families.first() << '"';
    if (!def.styleName.isEmpty())
        d << ", stylename=" << def.styleName;
    d << ", pointsize=" << def.pointSize << ", pixelsize=" << def.pixelSize
        << ", styleHint=" << def.styleHint << ", weight=" << def.weight
        << ", stretch=" << def.stretch << ", hintingPreference="
        << def.hintingPreference << ')';
    return d;
}

void QWindowsFontDatabase::debugFormat(QDebug &d, const LOGFONT &lf)
{
    d << "LOGFONT(\"" << QString::fromWCharArray(lf.lfFaceName)
        << "\", lfWidth=" << lf.lfWidth << ", lfHeight=" << lf.lfHeight << ')';
}

QDebug operator<<(QDebug d, const LOGFONT &lf)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d.noquote();
    QWindowsFontDatabase::debugFormat(d, lf);
    return d;
}
#endif // !QT_NO_DEBUG_STREAM

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

bool qt_localizedName(const QString &name)
{
    const QChar *c = name.unicode();
    for (int i = 0; i < name.length(); ++i) {
        if (c[i].unicode() >= 0x100)
            return true;
    }
    return false;
}

namespace {

static QString readName(bool unicode, const uchar *string, int length)
{
    QString out;
    if (unicode) {
        // utf16

        length /= 2;
        out.resize(length);
        QChar *uc = out.data();
        for (int i = 0; i < length; ++i)
            uc[i] = qt_getUShort(string + 2*i);
    } else {
        // Apple Roman

        out.resize(length);
        QChar *uc = out.data();
        for (int i = 0; i < length; ++i)
            uc[i] = QLatin1Char(char(string[i]));
    }
    return out;
}

enum FieldTypeValue {
    FamilyId = 1,
    StyleId = 2,
    PreferredFamilyId = 16,
    PreferredStyleId = 17,
};

enum PlatformFieldValue {
    PlatformId_Unicode = 0,
    PlatformId_Apple = 1,
    PlatformId_Microsoft = 3
};

QFontNames qt_getCanonicalFontNames(const uchar *table, quint32 bytes)
{
    QFontNames out;
    const int NameRecordSize = 12;
    const int MS_LangIdEnglish = 0x009;

    // get the name table
    quint16 count;
    quint16 string_offset;
    const unsigned char *names;

    if (bytes < 8)
        return out;

    if (qt_getUShort(table) != 0)
        return out;

    count = qt_getUShort(table + 2);
    string_offset = qt_getUShort(table + 4);
    names = table + 6;

    if (string_offset >= bytes || 6 + count*NameRecordSize > string_offset)
        return out;

    enum PlatformIdType {
        NotFound = 0,
        Unicode = 1,
        Apple = 2,
        Microsoft = 3
    };

    PlatformIdType idStatus[4] = { NotFound, NotFound, NotFound, NotFound };
    int ids[4] = { -1, -1, -1, -1 };

    for (int i = 0; i < count; ++i) {
        // search for the correct name entries

        quint16 platform_id = qt_getUShort(names + i*NameRecordSize);
        quint16 encoding_id = qt_getUShort(names + 2 + i*NameRecordSize);
        quint16 language_id = qt_getUShort(names + 4 + i*NameRecordSize);
        quint16 name_id = qt_getUShort(names + 6 + i*NameRecordSize);

        PlatformIdType *idType = nullptr;
        int *id = nullptr;

        switch (name_id) {
        case FamilyId:
            idType = &idStatus[0];
            id = &ids[0];
            break;
        case StyleId:
            idType = &idStatus[1];
            id = &ids[1];
            break;
        case PreferredFamilyId:
            idType = &idStatus[2];
            id = &ids[2];
            break;
        case PreferredStyleId:
            idType = &idStatus[3];
            id = &ids[3];
            break;
        default:
            continue;
        }

        quint16 length = qt_getUShort(names + 8 + i*NameRecordSize);
        quint16 offset = qt_getUShort(names + 10 + i*NameRecordSize);
        if (DWORD(string_offset + offset + length) > bytes)
            continue;

        if ((platform_id == PlatformId_Microsoft
            && (encoding_id == 0 || encoding_id == 1))
            && ((language_id & 0x3ff) == MS_LangIdEnglish
                || *idType < Microsoft)) {
            *id = i;
            *idType = Microsoft;
        }
        // not sure if encoding id 4 for Unicode is utf16 or ucs4...
        else if (platform_id == PlatformId_Unicode && encoding_id < 4 && *idType < Unicode) {
            *id = i;
            *idType = Unicode;
        }
        else if (platform_id == PlatformId_Apple && encoding_id == 0 && language_id == 0 && *idType < Apple) {
            *id = i;
            *idType = Apple;
        }
    }

    QString strings[4];
    for (int i = 0; i < 4; ++i) {
        if (idStatus[i] == NotFound)
            continue;
        int id = ids[i];
        quint16 length = qt_getUShort(names +  8 + id * NameRecordSize);
        quint16 offset = qt_getUShort(names + 10 + id * NameRecordSize);
        const unsigned char *string = table + string_offset + offset;
        strings[i] = readName(idStatus[i] != Apple, string, length);
    }

    out.name = strings[0];
    out.style = strings[1];
    out.preferredName = strings[2];
    out.preferredStyle = strings[3];
    return out;
}

} // namespace

QString qt_getEnglishName(const QString &familyName, bool includeStyle)
{
    QString i18n_name;
    QString faceName = familyName;
    faceName.truncate(LF_FACESIZE - 1);

    HDC hdc = GetDC( 0 );
    LOGFONT lf;
    memset(&lf, 0, sizeof(LOGFONT));
    faceName.toWCharArray(lf.lfFaceName);
    lf.lfFaceName[faceName.size()] = 0;
    lf.lfCharSet = DEFAULT_CHARSET;
    HFONT hfont = CreateFontIndirect(&lf);

    if (!hfont) {
        ReleaseDC(0, hdc);
        return QString();
    }

    HGDIOBJ oldobj = SelectObject( hdc, hfont );

    const DWORD name_tag = qFromBigEndian(QFont::Tag("name").value());

    // get the name table
    unsigned char *table = 0;

    DWORD bytes = GetFontData( hdc, name_tag, 0, 0, 0 );
    if ( bytes == GDI_ERROR ) {
        // ### Unused variable
        // int err = GetLastError();
        goto error;
    }

    table = new unsigned char[bytes];
    GetFontData(hdc, name_tag, 0, table, bytes);
    if ( bytes == GDI_ERROR )
        goto error;

    {
        const QFontNames names = qt_getCanonicalFontNames(table, bytes);
        i18n_name = names.name;
        if (includeStyle)
            i18n_name += u' ' + names.style;
    }
error:
    delete [] table;
    SelectObject( hdc, oldobj );
    DeleteObject( hfont );
    ReleaseDC( 0, hdc );

    //qDebug("got i18n name of '%s' for font '%s'", i18n_name.latin1(), familyName.toLocal8Bit().data());
    return i18n_name;
}

// Note this duplicates parts of qt_getEnglishName, we should try to unify the two functions.
QFontNames qt_getCanonicalFontNames(const LOGFONT &lf)
{
    QFontNames fontNames;
    HDC hdc = GetDC(0);
    HFONT hfont = CreateFontIndirect(&lf);

    if (!hfont) {
        ReleaseDC(0, hdc);
        return fontNames;
    }

    HGDIOBJ oldobj = SelectObject(hdc, hfont);

    // get the name table
    QByteArray table;
    const DWORD name_tag = qFromBigEndian(QFont::Tag("name").value());
    DWORD bytes = GetFontData(hdc, name_tag, 0, 0, 0);
    if (bytes != GDI_ERROR) {
        table.resize(bytes);

        if (GetFontData(hdc, name_tag, 0, table.data(), bytes) != GDI_ERROR)
            fontNames = qt_getCanonicalFontNames(reinterpret_cast<const uchar*>(table.constData()), bytes);
    }

    SelectObject(hdc, oldobj);
    DeleteObject(hfont);
    ReleaseDC(0, hdc);

    return fontNames;
}

namespace {
    struct StoreFontPayload {
        StoreFontPayload(const QString &family,
                         QWindowsFontDatabase *fontDatabase)
            : populatedFontFamily(family)
            , windowsFontDatabase(fontDatabase)
        {}

        QString populatedFontFamily;
        QDuplicateTracker<FontAndStyle> foundFontAndStyles;
        QWindowsFontDatabase *windowsFontDatabase;
    };
}

static bool addFontToDatabase(QString familyName,
                              QString styleName,
                              const LOGFONT &logFont,
                              const TEXTMETRIC *textmetric,
                              const FONTSIGNATURE *signature,
                              int type,
                              StoreFontPayload *sfp)
{
    // the "@family" fonts are just the same as "family". Ignore them.
    if (familyName.isEmpty() || familyName.at(0) == u'@' || familyName.startsWith("WST_"_L1))
        return false;

    uchar charSet = logFont.lfCharSet;

    static const int SMOOTH_SCALABLE = 0xffff;
    const QString foundryName; // No such concept.
    const bool fixed = !(textmetric->tmPitchAndFamily & TMPF_FIXED_PITCH);
    const bool ttf = (textmetric->tmPitchAndFamily & TMPF_TRUETYPE);
    const bool unreliableTextMetrics = type == 0;
    const bool scalable = (textmetric->tmPitchAndFamily & (TMPF_VECTOR|TMPF_TRUETYPE))
                          && !unreliableTextMetrics;
    const int size = scalable ? SMOOTH_SCALABLE : textmetric->tmHeight;
    const QFont::Style style = textmetric->tmItalic ? QFont::StyleItalic : QFont::StyleNormal;
    const bool antialias = false;
    const QFont::Weight weight = static_cast<QFont::Weight>(textmetric->tmWeight);
    const QFont::Stretch stretch = QFont::Unstretched;

#ifndef QT_NO_DEBUG_OUTPUT
    if (lcQpaFonts().isDebugEnabled()) {
        QString message;
        QTextStream str(&message);
        str << __FUNCTION__ << ' ' << familyName << ' ' << charSet << " TTF=" << ttf;
        if (type & DEVICE_FONTTYPE)
            str << " DEVICE";
        if (type & RASTER_FONTTYPE)
            str << " RASTER";
        if (type & TRUETYPE_FONTTYPE)
            str << " TRUETYPE";
        str << " scalable=" << scalable << " Size=" << size
                << " Style=" << style << " Weight=" << weight
                << " stretch=" << stretch << " styleName=" << styleName;
        qCDebug(lcQpaFonts) << message;
    }
#endif
    QString englishName;
    QString faceName;

    QString subFamilyName;
    QString subFamilyStyle;
    // Look-up names registered in the font
    QFontNames canonicalNames = qt_getCanonicalFontNames(logFont);
    if (qt_localizedName(familyName) && !canonicalNames.name.isEmpty())
        englishName = canonicalNames.name;
    if (!canonicalNames.preferredName.isEmpty()) {
        subFamilyName = familyName;
        subFamilyStyle = styleName;
        faceName = familyName; // Remember the original name for later lookups
        familyName = canonicalNames.preferredName;
        // Preferred style / typographic subfamily name:
        // "If it is absent, then name ID 2 is considered to be the typographic subfamily name."
        // From: https://docs.microsoft.com/en-us/windows/win32/directwrite/opentype-variable-fonts
        // Name ID 2 is already stored in the styleName variable. Furthermore, for variable fonts,
        // styleName holds the variation instance name, which should be used over name ID 2.
        if (!canonicalNames.preferredStyle.isEmpty())
            styleName = canonicalNames.preferredStyle;
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
                familyName == "Segoe UI"_L1)
            writingSystems.setSupported(QFontDatabase::Thai, false);
    } else {
        const QFontDatabase::WritingSystem ws = writingSystemFromCharSet(charSet);
        if (ws != QFontDatabase::Any)
            writingSystems.setSupported(ws);
    }

    const bool wasPopulated = QPlatformFontDatabase::isFamilyPopulated(familyName);
    QPlatformFontDatabase::registerFont(familyName, styleName, foundryName, weight,
                                        style, stretch, antialias, scalable, size, fixed, false, writingSystems, new QWindowsFontDatabase::FontHandle(faceName));


    // add fonts windows can generate for us:
    if (weight <= QFont::DemiBold && styleName.isEmpty())
        QPlatformFontDatabase::registerFont(familyName, QString(), foundryName, QFont::Bold,
                                            style, stretch, antialias, scalable, size, fixed, false, writingSystems, new QWindowsFontDatabase::FontHandle(faceName));
    if (style != QFont::StyleItalic && styleName.isEmpty())
        QPlatformFontDatabase::registerFont(familyName, QString(), foundryName, weight,
                                            QFont::StyleItalic, stretch, antialias, scalable, size, fixed, false, writingSystems, new QWindowsFontDatabase::FontHandle(faceName));
    if (weight <= QFont::DemiBold && style != QFont::StyleItalic && styleName.isEmpty())
        QPlatformFontDatabase::registerFont(familyName, QString(), foundryName, QFont::Bold,
                                            QFont::StyleItalic, stretch, antialias, scalable, size, fixed, false, writingSystems, new QWindowsFontDatabase::FontHandle(faceName));

    // We came here from populating a different font family, so we have
    // to ensure the entire typographic family is populated before we
    // mark it as such inside registerFont()
    if (!subFamilyName.isEmpty()
            && familyName != subFamilyName
            && sfp->populatedFontFamily != familyName
            && !wasPopulated) {
        sfp->windowsFontDatabase->populateFamily(familyName);
    }

    if (!subFamilyName.isEmpty() && familyName != subFamilyName) {
        QPlatformFontDatabase::registerFont(subFamilyName, subFamilyStyle, foundryName, weight,
                                            style, stretch, antialias, scalable, size, fixed, false, writingSystems, new QWindowsFontDatabase::FontHandle(faceName));
    }

    if (!englishName.isEmpty() && englishName != familyName)
        QPlatformFontDatabase::registerAliasToFontFamily(familyName, englishName);

    return true;
}

static int QT_WIN_CALLBACK storeFont(const LOGFONT *logFont, const TEXTMETRIC *textmetric,
                                     DWORD type, LPARAM lparam)
{
    const ENUMLOGFONTEX *f = reinterpret_cast<const ENUMLOGFONTEX *>(logFont);
    const QString familyName = QString::fromWCharArray(f->elfLogFont.lfFaceName);
    const QString styleName = QString::fromWCharArray(f->elfStyle);

    // NEWTEXTMETRICEX (passed for TT fonts) is a NEWTEXTMETRIC, which according
    // to the documentation is identical to a TEXTMETRIC except for the last four
    // members, which we don't use anyway
    const FONTSIGNATURE *signature = nullptr;
    StoreFontPayload *sfp = reinterpret_cast<StoreFontPayload *>(lparam);
    Q_ASSERT(sfp != nullptr);
    if (type & TRUETYPE_FONTTYPE) {
        signature = &reinterpret_cast<const NEWTEXTMETRICEX *>(textmetric)->ntmFontSig;
        // We get a callback for each script-type supported, but we register them all
        // at once using the signature, so we only need one call to addFontToDatabase().
        if (sfp->foundFontAndStyles.hasSeen({familyName, styleName}))
            return 1;
    }
    addFontToDatabase(familyName, styleName, *logFont, textmetric, signature, type, sfp);

    // keep on enumerating
    return 1;
}

bool QWindowsFontDatabase::populateFamilyAliases(const QString &missingFamily)
{
    Q_UNUSED(missingFamily);

    if (m_hasPopulatedAliases)
        return false;

    QStringList families = QFontDatabase::families();
    for (const QString &family : families)
        populateFamily(family);
    m_hasPopulatedAliases = true;

    return true;
}

void QWindowsFontDatabase::populateFamily(const QString &familyName)
{
    qCDebug(lcQpaFonts) << familyName;
    if (familyName.size() >= LF_FACESIZE) { // Field length of LOGFONT::lfFaceName
        qCDebug(lcQpaFonts) << "Unable to enumerate family '" << familyName << '\'';
        return;
    }
    HDC dummy = GetDC(0);
    LOGFONT lf;
    lf.lfCharSet = DEFAULT_CHARSET;
    familyName.toWCharArray(lf.lfFaceName);
    lf.lfFaceName[familyName.size()] = 0;
    lf.lfPitchAndFamily = 0;
    StoreFontPayload sfp(familyName, this);
    EnumFontFamiliesEx(dummy, &lf, storeFont, reinterpret_cast<intptr_t>(&sfp), 0);
    ReleaseDC(0, dummy);
}

static int QT_WIN_CALLBACK populateFontFamilies(const LOGFONT *logFont, const TEXTMETRIC *textmetric,
                                                DWORD, LPARAM)
{
    // the "@family" fonts are just the same as "family". Ignore them.
    const ENUMLOGFONTEX *f = reinterpret_cast<const ENUMLOGFONTEX *>(logFont);
    const wchar_t *faceNameW = f->elfLogFont.lfFaceName;
    if (faceNameW[0] && faceNameW[0] != L'@' && wcsncmp(faceNameW, L"WST_", 4)) {
        const QString faceName = QString::fromWCharArray(faceNameW);
        QPlatformFontDatabase::registerFontFamily(faceName);
        // Register current font's english name as alias
        const bool ttf = (textmetric->tmPitchAndFamily & TMPF_TRUETYPE);
        if (ttf && qt_localizedName(faceName)) {
            const QString englishName = qt_getEnglishName(faceName);
            if (!englishName.isEmpty())
                QPlatformFontDatabase::registerAliasToFontFamily(faceName, englishName);
        }
    }
    return 1; // continue
}

namespace {

QString resolveFontPath(const QString &fontPath)
{
    if (fontPath.isEmpty())
        return QString();

    if (QFile::exists(fontPath))
        return fontPath;

    // resolve the path relatively to Windows Fonts directory
    return QStandardPaths::locate(QStandardPaths::FontsLocation, fontPath);
}

}

void QWindowsFontDatabase::addDefaultEUDCFont()
{
    const QString path = QWinRegistryKey(HKEY_CURRENT_USER, LR"(EUDC\1252)")
                         .stringValue(L"SystemDefaultEUDCFont");
    if (path.isEmpty()) {
        qCDebug(lcQpaFonts) << "There's no default EUDC font specified";
        return;
    }

    const QString absolutePath = resolveFontPath(path);
    if (absolutePath.isEmpty()) {
        qCDebug(lcQpaFonts) << "Unable to locate default EUDC font:" << path;
        return;
    }

    QFile file(absolutePath);
    if (!file.open(QIODevice::ReadOnly)) {
        qCWarning(lcQpaFonts) << "Unable to open default EUDC font:" << absolutePath;
        return;
    }

    m_eudcFonts = addApplicationFont(file.readAll(), absolutePath);
}

void QWindowsFontDatabase::populateFontDatabase()
{
    HDC dummy = GetDC(0);
    LOGFONT lf;
    lf.lfCharSet = DEFAULT_CHARSET;
    lf.lfFaceName[0] = 0;
    lf.lfPitchAndFamily = 0;
    EnumFontFamiliesEx(dummy, &lf, populateFontFamilies, 0, 0);
    ReleaseDC(0, dummy);
    // Work around EnumFontFamiliesEx() not listing the system font.
    const QString systemDefaultFamily = QWindowsFontDatabase::systemDefaultFont().families().constFirst();
    if (QPlatformFontDatabase::resolveFontFamilyAlias(systemDefaultFamily) == systemDefaultFamily)
        QPlatformFontDatabase::registerFontFamily(systemDefaultFamily);
    addDefaultEUDCFont();
}

void QWindowsFontDatabase::invalidate()
{
    QWindowsFontDatabaseBase::invalidate();
    removeApplicationFonts();
}

QWindowsFontDatabase::QWindowsFontDatabase()
{
    // Properties accessed by QWin32PrintEngine (Qt Print Support)
    static const int hfontMetaTypeId = qRegisterMetaType<HFONT>();
    static const int logFontMetaTypeId = qRegisterMetaType<LOGFONT>();
    Q_UNUSED(hfontMetaTypeId);
    Q_UNUSED(logFontMetaTypeId);

    if (lcQpaFonts().isDebugEnabled()) {
        QSharedPointer<QWindowsFontEngineData> d = data();
        qCDebug(lcQpaFonts) << __FUNCTION__ << "Clear type: "
            << d->clearTypeEnabled << "gamma: " << d->fontSmoothingGamma;
    }
}

QWindowsFontDatabase::~QWindowsFontDatabase()
{
    removeApplicationFonts();
}

QFontEngine * QWindowsFontDatabase::fontEngine(const QFontDef &fontDef, void *handle)
{
    FontHandle *fontHandle = static_cast<FontHandle *>(handle);
    const QString faceName = fontHandle->faceName.left(LF_FACESIZE - 1);
    QFontEngine *fe = QWindowsFontDatabase::createEngine(fontDef, faceName,
                                                         defaultVerticalDPI(),
                                                         data());
    qCDebug(lcQpaFonts) << __FUNCTION__ << "FONTDEF" << fontDef << fe << handle;
    return fe;
}

QFontEngine *QWindowsFontDatabase::fontEngine(const QByteArray &fontData, qreal pixelSize, QFont::HintingPreference hintingPreference)
{
    EmbeddedFont font(fontData);
    QFontEngine *fontEngine = 0;

#if QT_CONFIG(directwrite)
    if (!useDirectWrite(hintingPreference))
#endif
    {
        GUID guid;
        CoCreateGuid(&guid);

QT_WARNING_PUSH
QT_WARNING_DISABLE_GCC("-Wstrict-aliasing")
        QString uniqueFamilyName = u'f'
                + QString::number(guid.Data1, 36) + u'-'
                + QString::number(guid.Data2, 36) + u'-'
                + QString::number(guid.Data3, 36) + u'-'
                + QString::number(*reinterpret_cast<quint64 *>(guid.Data4), 36);
QT_WARNING_POP

        QString actualFontName = font.changeFamilyName(uniqueFamilyName);
        if (actualFontName.isEmpty()) {
            qCWarning(lcQpaFonts, "%s: Can't change family name of font", __FUNCTION__);
            return 0;
        }

        DWORD count = 0;
        QByteArray newFontData = font.data();
        HANDLE fontHandle =
            AddFontMemResourceEx(const_cast<char *>(newFontData.constData()),
                                 DWORD(newFontData.size()), 0, &count);
        if (count == 0 && fontHandle != 0) {
            RemoveFontMemResourceEx(fontHandle);
            fontHandle = 0;
        }

        if (fontHandle == 0) {
            qCWarning(lcQpaFonts, "%s: AddFontMemResourceEx failed", __FUNCTION__);
        } else {
            QFontDef request;
            request.families = QStringList(uniqueFamilyName);
            request.pixelSize = pixelSize;
            request.styleStrategy = QFont::PreferMatch;
            request.hintingPreference = hintingPreference;
            request.stretch = QFont::Unstretched;

            fontEngine = QWindowsFontDatabase::createEngine(request, QString(),
                                                            defaultVerticalDPI(),
                                                            data());

            if (fontEngine) {
                if (request.families != fontEngine->fontDef.families) {
                    qCWarning(lcQpaFonts, "%s: Failed to load font. Got fallback instead: %s", __FUNCTION__,
                             qPrintable(fontEngine->fontDef.families.constFirst()));
                    if (fontEngine->ref.loadRelaxed() == 0)
                        delete fontEngine;
                    fontEngine = 0;
                } else {
                    Q_ASSERT(fontEngine->ref.loadRelaxed() == 0);

                    // Override the generated font name
                    switch (fontEngine->type()) {
                    case QFontEngine::Win:
                        static_cast<QWindowsFontEngine *>(fontEngine)->setUniqueFamilyName(uniqueFamilyName);
                        fontEngine->fontDef.families = QStringList(actualFontName);
                        break;
#if QT_CONFIG(directwrite) && QT_CONFIG(direct2d)
                    case QFontEngine::DirectWrite:
                        static_cast<QWindowsFontEngineDirectWrite *>(fontEngine)->setUniqueFamilyName(uniqueFamilyName);
                        fontEngine->fontDef.families = QStringList(actualFontName);
                        break;
#endif // directwrite && direct2d

                    default:
                        Q_ASSERT_X(false, Q_FUNC_INFO, "Unhandled font engine.");
                    }

                    UniqueFontData uniqueData{};
                    uniqueData.handle = fontHandle;
                    ++uniqueData.refCount;
                    {
                        const std::scoped_lock lock(m_uniqueFontDataMutex);
                        m_uniqueFontData[uniqueFamilyName] = uniqueData;
                    }
                }
            } else {
                RemoveFontMemResourceEx(fontHandle);
            }
        }

        // Get style and weight info
        if (fontEngine != nullptr)
            font.updateFromOS2Table(fontEngine);
    }
#if QT_CONFIG(directwrite) && QT_CONFIG(direct2d)
    else {
        fontEngine = QWindowsFontDatabaseBase::fontEngine(fontData, pixelSize, hintingPreference);
    }
#endif // directwrite && direct2d

    qCDebug(lcQpaFonts) << __FUNCTION__ << "FONTDATA" << fontData << pixelSize << hintingPreference << fontEngine;
    return fontEngine;
}

static QList<quint32> getTrueTypeFontOffsets(const uchar *fontData, const uchar *fileEndSentinel)
{
    QList<quint32> offsets;
    if (fileEndSentinel - fontData < 12) {
        qCWarning(lcQpaFonts) << "Corrupted font data detected";
        return offsets;
    }

    const quint32 headerTag = qFromUnaligned<quint32>(fontData);
    if (headerTag != qFromBigEndian(QFont::Tag("ttcf").value())) {
        if (headerTag != qFromBigEndian(QFont::Tag("\0\1\0\0").value())
            && headerTag != qFromBigEndian(QFont::Tag("OTTO").value())
            && headerTag != qFromBigEndian(QFont::Tag("true").value())
            && headerTag != qFromBigEndian(QFont::Tag("typ1").value())) {
            return offsets;
        }
        offsets << 0;
        return offsets;
    }

    const quint32 maximumNumFonts = 0xffff;
    const quint32 numFonts = qFromBigEndian<quint32>(fontData + 8);
    if (numFonts > maximumNumFonts) {
        qCWarning(lcQpaFonts) << "Font collection of" << numFonts << "fonts is too large. Aborting.";
        return offsets;
    }

    if (quintptr(fileEndSentinel - fontData) > 12 + (numFonts - 1) * 4) {
        for (quint32 i = 0; i < numFonts; ++i)
            offsets << qFromBigEndian<quint32>(fontData + 12 + i * 4);
    } else {
        qCWarning(lcQpaFonts) << "Corrupted font data detected";
    }

    return offsets;
}

static void getFontTable(const uchar *fileBegin, const uchar *fileEndSentinel, const uchar *data, quint32 tag, const uchar **table, quint32 *length)
{
    if (fileEndSentinel - data >= 6) {
        const quint16 numTables = qFromBigEndian<quint16>(data + 4);
        if (fileEndSentinel - data >= 28 + 16 * (numTables - 1)) {
            for (quint32 i = 0; i < numTables; ++i) {
                const quint32 offset = 12 + 16 * i;
                if (qFromUnaligned<quint32>(data + offset) == tag) {
                    const quint32 tableOffset = qFromBigEndian<quint32>(data + offset + 8);
                    if (quintptr(fileEndSentinel - fileBegin) <= tableOffset) {
                        qCWarning(lcQpaFonts) << "Corrupted font data detected";
                        break;
                    }
                    *table = fileBegin + tableOffset;
                    *length = qFromBigEndian<quint32>(data + offset + 12);
                    if (quintptr(fileEndSentinel - *table) < *length) {
                        qCWarning(lcQpaFonts) << "Corrupted font data detected";
                        break;
                    }
                    return;
                }
            }
        } else {
            qCWarning(lcQpaFonts) << "Corrupted font data detected";
        }
    } else {
        qCWarning(lcQpaFonts) << "Corrupted font data detected";
    }
    *table = 0;
    *length = 0;
    return;
}

static void getFamiliesAndSignatures(const QByteArray &fontData,
                                     QList<QFontNames> *families,
                                     QList<FONTSIGNATURE> *signatures,
                                     QList<QFontValues> *values)
{
    const uchar *data = reinterpret_cast<const uchar *>(fontData.constData());
    const uchar *dataEndSentinel = data + fontData.size();

    QList<quint32> offsets = getTrueTypeFontOffsets(data, dataEndSentinel);
    if (offsets.isEmpty())
        return;

    for (int i = 0; i < offsets.count(); ++i) {
        const uchar *font = data + offsets.at(i);
        const uchar *table;
        quint32 length;
        getFontTable(data, dataEndSentinel, font,
                     qFromBigEndian(QFont::Tag("name").value()),
                     &table, &length);
        if (!table)
            continue;
        QFontNames names = qt_getCanonicalFontNames(table, length);
        if (names.name.isEmpty())
            continue;

        families->append(std::move(names));

        if (values || signatures) {
            getFontTable(data, dataEndSentinel, font,
                         qFromBigEndian(QFont::Tag("OS/2").value()),
                         &table, &length);
        }

        if (values) {
            QFontValues fontValues;
            if (table && length >= 64) {
                // Read in some details about the font, offset calculated based on the specification
                fontValues.weight = qFromBigEndian<quint16>(table + 4);

                quint16 fsSelection = qFromBigEndian<quint16>(table + 62);
                fontValues.isItalic = (fsSelection & 1) != 0;
                fontValues.isUnderlined = (fsSelection & (1 << 1)) != 0;
                fontValues.isOverstruck = (fsSelection & (1 << 4)) != 0;
            }
            values->append(std::move(fontValues));
        }

        if (signatures) {
            FONTSIGNATURE signature;
            if (table && length >= 86) {
                // Offsets taken from OS/2 table in the TrueType spec
                signature.fsUsb[0] = qFromBigEndian<quint32>(table + 42);
                signature.fsUsb[1] = qFromBigEndian<quint32>(table + 46);
                signature.fsUsb[2] = qFromBigEndian<quint32>(table + 50);
                signature.fsUsb[3] = qFromBigEndian<quint32>(table + 54);

                signature.fsCsb[0] = qFromBigEndian<quint32>(table + 78);
                signature.fsCsb[1] = qFromBigEndian<quint32>(table + 82);
            } else {
                memset(&signature, 0, sizeof(signature));
            }
            signatures->append(signature);
        }
    }
}

QStringList QWindowsFontDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName, QFontDatabasePrivate::ApplicationFont *applicationFont)
{
    WinApplicationFont font;
    font.fileName = fileName;
    QList<FONTSIGNATURE> signatures;
    QList<QFontValues> fontValues;
    QList<QFontNames> families;
    QStringList familyNames;

    if (!fontData.isEmpty()) {
        getFamiliesAndSignatures(fontData, &families, &signatures, &fontValues);
        if (families.isEmpty())
            return familyNames;

        DWORD dummy = 0;
        font.handle =
            AddFontMemResourceEx(const_cast<char *>(fontData.constData()),
                                 DWORD(fontData.size()), 0, &dummy);
        if (font.handle == 0)
            return QStringList();

        // Memory fonts won't show up in enumeration, so do add them the hard way.
        for (int j = 0; j < families.count(); ++j) {
            const auto &family = families.at(j);
            const QString &familyName = family.name;
            const QString &styleName = family.style;
            familyNames << familyName;
            HDC hdc = GetDC(0);
            LOGFONT lf;
            memset(&lf, 0, sizeof(LOGFONT));
            memcpy(lf.lfFaceName, familyName.data(), sizeof(wchar_t) * qMin(LF_FACESIZE - 1, familyName.size()));
            lf.lfCharSet = DEFAULT_CHARSET;
            const QFontValues &values = fontValues.at(j);
            lf.lfWeight = values.weight;
            if (values.isItalic)
                lf.lfItalic = TRUE;
            if (values.isOverstruck)
                lf.lfStrikeOut = TRUE;
            if (values.isUnderlined)
                lf.lfUnderline = TRUE;
            HFONT hfont = CreateFontIndirect(&lf);
            HGDIOBJ oldobj = SelectObject(hdc, hfont);

            if (applicationFont != nullptr) {
                QFontDatabasePrivate::ApplicationFont::Properties properties;
                properties.style = values.isItalic ? QFont::StyleItalic : QFont::StyleNormal;
                properties.weight = static_cast<int>(values.weight);
                properties.familyName = familyName;
                properties.styleName = styleName;

                applicationFont->properties.append(properties);
            }

            TEXTMETRIC textMetrics;
            GetTextMetrics(hdc, &textMetrics);

            StoreFontPayload sfp(familyName, this);
            addFontToDatabase(familyName, styleName, lf, &textMetrics, &signatures.at(j),
                              TRUETYPE_FONTTYPE, &sfp);

            SelectObject(hdc, oldobj);
            DeleteObject(hfont);
            ReleaseDC(0, hdc);
        }
    } else {
        QFile f(fileName);
        if (!f.open(QIODevice::ReadOnly))
            return QStringList();
        QByteArray data = f.readAll();
        f.close();


        getFamiliesAndSignatures(data, &families, nullptr, applicationFont != nullptr ? &fontValues : nullptr);

        if (families.isEmpty())
            return QStringList();

        if (AddFontResourceExW((wchar_t*)fileName.utf16(), FR_PRIVATE, 0) == 0)
            return QStringList();

        font.handle = 0;

        // Fonts based on files are added via populate, as they will show up in font enumeration.
        for (int j = 0; j < families.count(); ++j) {
            const QString familyName = families.at(j).name;
            familyNames << familyName;

            if (applicationFont != nullptr) {
                const QString &styleName = families.at(j).style;
                const QFontValues &values = fontValues.at(j);

                QFontDatabasePrivate::ApplicationFont::Properties properties;
                properties.style = values.isItalic ? QFont::StyleItalic : QFont::StyleNormal;
                properties.weight = static_cast<int>(values.weight);
                properties.familyName = familyName;
                properties.styleName = styleName;

                applicationFont->properties.append(properties);
            }

            populateFamily(familyName);
        }
    }

    m_applicationFonts << font;

    return familyNames;
}

void QWindowsFontDatabase::removeApplicationFonts()
{
    for (const WinApplicationFont &font : std::as_const(m_applicationFonts)) {
        if (font.handle) {
            RemoveFontMemResourceEx(font.handle);
        } else {
            RemoveFontResourceExW(reinterpret_cast<LPCWSTR>(font.fileName.utf16()),
                                  FR_PRIVATE, nullptr);
        }
    }
    m_applicationFonts.clear();
    m_eudcFonts.clear();
}

QWindowsFontDatabase::FontHandle::FontHandle(IDWriteFontFace *face, const QString &name)
    : fontFace(face), faceName(name)
{
    fontFace->AddRef();
}


QWindowsFontDatabase::FontHandle::~FontHandle()
{
    if (fontFace != nullptr)
        fontFace->Release();
}

void QWindowsFontDatabase::releaseHandle(void *handle)
{
    delete static_cast<FontHandle *>(handle);
}

QString QWindowsFontDatabase::fontDir() const
{
    const QString result = QPlatformFontDatabase::fontDir();
    qCDebug(lcQpaFonts) << __FUNCTION__ << result;
    return result;
}

bool QWindowsFontDatabase::fontsAlwaysScalable() const
{
    return false;
}

void QWindowsFontDatabase::derefUniqueFont(const QString &uniqueFont)
{
    const std::scoped_lock lock(m_uniqueFontDataMutex);
    const auto it = m_uniqueFontData.find(uniqueFont);
    if (it != m_uniqueFontData.end()) {
        if (--it->refCount == 0) {
            RemoveFontMemResourceEx(it->handle);
            m_uniqueFontData.erase(it);
        }
    }
}

void QWindowsFontDatabase::refUniqueFont(const QString &uniqueFont)
{
    const std::scoped_lock lock(m_uniqueFontDataMutex);
    const auto it = m_uniqueFontData.find(uniqueFont);
    if (it != m_uniqueFontData.end())
        ++it->refCount;
}

QStringList QWindowsFontDatabase::fallbacksForFamily(const QString &family,
                                                     QFont::Style style,
                                                     QFont::StyleHint styleHint,
                                                     QFontDatabasePrivate::ExtendedScript script) const
{
    QStringList result;
    result.append(QWindowsFontDatabaseBase::familiesForScript(script));
    result.append(familyForStyleHint(styleHint));
    result.append(m_eudcFonts);
    result.append(extraTryFontsForFamily(family));
    result.append(QPlatformFontDatabase::fallbacksForFamily(family, style, styleHint, script));

    qCDebug(lcQpaFonts) << __FUNCTION__ << family << style << styleHint
        << script << result;
    return result;
}


QFontEngine *QWindowsFontDatabase::createEngine(const QFontDef &request, const QString &faceName,
                                                int dpi,
                                                const QSharedPointer<QWindowsFontEngineData> &data)
{
    QFontEngine *fe = nullptr;

    LOGFONT lf = fontDefToLOGFONT(request, faceName);
    const bool preferClearTypeAA = lf.lfQuality == CLEARTYPE_QUALITY;

    if (request.stretch != 100) {
        HFONT hfont = CreateFontIndirect(&lf);
        if (!hfont) {
            qErrnoWarning("%s: CreateFontIndirect failed", __FUNCTION__);
            hfont = QWindowsFontDatabase::systemFont();
        }

        HGDIOBJ oldObj = SelectObject(data->hdc, hfont);
        TEXTMETRIC tm;
        if (!GetTextMetrics(data->hdc, &tm))
            qErrnoWarning("%s: GetTextMetrics failed", __FUNCTION__);
        else
            lf.lfWidth = tm.tmAveCharWidth * request.stretch / 100;
        SelectObject(data->hdc, oldObj);

        DeleteObject(hfont);
    }

#if QT_CONFIG(directwrite) && QT_CONFIG(direct2d)
    if (data->directWriteFactory != nullptr) {
        const QString fam = QString::fromWCharArray(lf.lfFaceName);
        const QString nameSubstitute = QWindowsFontEngineDirectWrite::fontNameSubstitute(fam);
        if (nameSubstitute != fam) {
            const int nameSubstituteLength = qMin(nameSubstitute.length(), LF_FACESIZE - 1);
            memcpy(lf.lfFaceName, nameSubstitute.data(), nameSubstituteLength * sizeof(wchar_t));
            lf.lfFaceName[nameSubstituteLength] = 0;
        }

        HFONT hfont = CreateFontIndirect(&lf);
        if (!hfont) {
            qErrnoWarning("%s: CreateFontIndirect failed", __FUNCTION__);
        } else {
            HGDIOBJ oldFont = SelectObject(data->hdc, hfont);

            const QFont::HintingPreference hintingPreference =
                static_cast<QFont::HintingPreference>(request.hintingPreference);
            bool useDw = useDirectWrite(hintingPreference, fam);

            IDWriteFontFace *directWriteFontFace = NULL;
            HRESULT hr = data->directWriteGdiInterop->CreateFontFaceFromHdc(data->hdc, &directWriteFontFace);
            if (SUCCEEDED(hr)) {
                bool isColorFont = false;
                bool needsSimulation = false;
#if QT_CONFIG(direct2d)
                IDWriteFontFace2 *directWriteFontFace2 = nullptr;
                if (SUCCEEDED(directWriteFontFace->QueryInterface(__uuidof(IDWriteFontFace2),
                                                                  reinterpret_cast<void **>(&directWriteFontFace2)))) {
                    if (directWriteFontFace2->IsColorFont())
                        isColorFont = directWriteFontFace2->GetPaletteEntryCount() > 0;

                    needsSimulation = directWriteFontFace2->GetSimulations() != DWRITE_FONT_SIMULATIONS_NONE;

                    directWriteFontFace2->Release();
                }
#endif // direct2d
                useDw = useDw || useDirectWrite(hintingPreference, fam, isColorFont) || needsSimulation;
                qCDebug(lcQpaFonts)
                        << __FUNCTION__ << request.families.first() << request.pointSize << "pt"
                        << "hintingPreference=" << hintingPreference << "color=" << isColorFont
                        << dpi << "dpi"
                        << "useDirectWrite=" << useDw;
                if (useDw) {
                    QWindowsFontEngineDirectWrite *fedw = new QWindowsFontEngineDirectWrite(directWriteFontFace,
                                                                                            request.pixelSize,
                                                                                            data);

                    wchar_t n[64];
                    GetTextFace(data->hdc, 64, n);

                    QFontDef fontDef = request;
                    fontDef.families = QStringList(QString::fromWCharArray(n));
                    fedw->initFontInfo(fontDef, dpi);
                    fe = fedw;
                }
                directWriteFontFace->Release();
            } else if (useDw) {
                const QString errorString = qt_error_string(int(hr));
                qCWarning(lcQpaFonts).noquote().nospace() << "DirectWrite: CreateFontFaceFromHDC() failed ("
                    << errorString << ") for " << request << ' ' << lf << " dpi=" << dpi;
            }

            SelectObject(data->hdc, oldFont);
            DeleteObject(hfont);
        }
    }
#endif // directwrite direct2d

    if (!fe) {
        QWindowsFontEngine *few = new QWindowsFontEngine(request.families.first(), lf, data);
        if (preferClearTypeAA)
            few->glyphFormat = QFontEngine::Format_A32;
        few->initFontInfo(request, dpi);
        fe = few;
    }

    return fe;
}

bool QWindowsFontDatabase::isPrivateFontFamily(const QString &family) const
{
    return m_eudcFonts.contains(family) || QPlatformFontDatabase::isPrivateFontFamily(family);
}

bool QWindowsFontDatabase::supportsColrv0Fonts() const
{
    return true;
}

QT_END_NAMESPACE
