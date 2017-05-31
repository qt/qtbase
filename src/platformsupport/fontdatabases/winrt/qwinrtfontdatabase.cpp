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

#include "qwinrtfontdatabase_p.h"

#include <QtFontDatabaseSupport/private/qfontengine_ft_p.h>

#include <QtCore/QCoreApplication>
#include <QtCore/QFile>

#include <QtCore/QUuid>
#include <dwrite_1.h>
#include <wrl.h>
using namespace Microsoft::WRL;

QT_BEGIN_NAMESPACE

Q_LOGGING_CATEGORY(lcQpaFonts, "qt.qpa.fonts")

QDebug operator<<(QDebug d, const QFontDef &def)
{
    QDebugStateSaver saver(d);
    d.nospace();
    d << "Family=" << def.family << " Stylename=" << def.styleName
        << " pointsize=" << def.pointSize << " pixelsize=" << def.pixelSize
        << " styleHint=" << def.styleHint << " weight=" << def.weight
        << " stretch=" << def.stretch << " hintingPreference="
        << def.hintingPreference;
    return d;
}

// Based on unicode range tables at http://www.microsoft.com/typography/otspec/os2.htm#ur
static QFontDatabase::WritingSystem writingSystemFromUnicodeRange(const DWRITE_UNICODE_RANGE &range)
{
    if (range.first >= 0x0000 && range.last <= 0x007F)
        return QFontDatabase::Latin;
    if (range.first >= 0x0370 && range.last <= 0x03FF)
        return QFontDatabase::Greek;
    if (range.first >= 0x0400 && range.last <= 0x04FF)
        return QFontDatabase::Cyrillic;
    if (range.first >= 0x0530 && range.last <= 0x058F)
        return QFontDatabase::Armenian;
    if (range.first >= 0x0590 && range.last <= 0x05FF)
        return QFontDatabase::Hebrew;
    if (range.first >= 0x0600 && range.last <= 0x06FF)
        return QFontDatabase::Arabic;
    if (range.first >= 0x0700 && range.last <= 0x074F)
        return QFontDatabase::Syriac;
    if (range.first >= 0x0780 && range.last <= 0x07BF)
        return QFontDatabase::Thaana;
    if (range.first >= 0x0900 && range.last <= 0x097F)
        return QFontDatabase::Devanagari;
    if (range.first >= 0x0980 && range.last <= 0x09FF)
        return QFontDatabase::Bengali;
    if (range.first >= 0x0A00 && range.last <= 0x0A7F)
        return QFontDatabase::Gurmukhi;
    if (range.first >= 0x0A80 && range.last <= 0x0AFF)
        return QFontDatabase::Gujarati;
    if (range.first >= 0x0B00 && range.last <= 0x0B7F)
        return QFontDatabase::Oriya;
    if (range.first >= 0x0B80 && range.last <= 0x0BFF)
        return QFontDatabase::Tamil;
    if (range.first >= 0x0C00 && range.last <= 0x0C7F)
        return QFontDatabase::Telugu;
    if (range.first >= 0x0C80 && range.last <= 0x0CFF)
        return QFontDatabase::Kannada;
    if (range.first >= 0x0D00 && range.last <= 0x0D7F)
        return QFontDatabase::Malayalam;
    if (range.first >= 0x0D80 && range.last <= 0x0DFF)
        return QFontDatabase::Sinhala;
    if (range.first >= 0x0E00 && range.last <= 0x0E7F)
        return QFontDatabase::Thai;
    if (range.first >= 0x0E80 && range.last <= 0x0EFF)
        return QFontDatabase::Lao;
    if (range.first >= 0x0F00 && range.last <= 0x0FFF)
        return QFontDatabase::Tibetan;
    if (range.first >= 0x1000 && range.last <= 0x109F)
        return QFontDatabase::Myanmar;
    if (range.first >= 0x10A0 && range.last <= 0x10FF)
        return QFontDatabase::Georgian;
    if (range.first >= 0x1780 && range.last <= 0x17FF)
        return QFontDatabase::Khmer;
    if (range.first >= 0x4E00 && range.last <= 0x9FFF)
        return QFontDatabase::SimplifiedChinese;
    if (range.first >= 0xAC00 && range.last <= 0xD7AF)
        return QFontDatabase::Korean;
    if (range.first >= 0x1680 && range.last <= 0x169F)
        return QFontDatabase::Ogham;
    if (range.first >= 0x16A0 && range.last <= 0x16FF)
        return QFontDatabase::Runic;
    if (range.first >= 0x07C0 && range.last <= 0x07FF)
        return QFontDatabase::Nko;

    return QFontDatabase::Other;
}

QWinRTFontDatabase::~QWinRTFontDatabase()
{
    qCDebug(lcQpaFonts) << __FUNCTION__;

    foreach (IDWriteFontFile *fontFile, m_fonts.keys())
        fontFile->Release();

    foreach (IDWriteFontFamily *fontFamily, m_fontFamilies)
        fontFamily->Release();
}

QString QWinRTFontDatabase::fontDir() const
{
    qCDebug(lcQpaFonts) << __FUNCTION__;
    QString fontDirectory = QFreeTypeFontDatabase::fontDir();
    if (!QFile::exists(fontDirectory)) {
        // Fall back to app directory + fonts, and just app directory after that
        const QString applicationDirPath = QCoreApplication::applicationDirPath();
        fontDirectory = applicationDirPath + QLatin1String("/fonts");
        if (!QFile::exists(fontDirectory)) {
            if (m_fontFamilies.isEmpty())
                qWarning("No fonts directory found in application package.");
            fontDirectory = applicationDirPath;
        }
    }
    return fontDirectory;
}

QFont QWinRTFontDatabase::defaultFont() const
{
    return QFont(QStringLiteral("Segoe UI"));
}

bool QWinRTFontDatabase::fontsAlwaysScalable() const
{
    return true;
}

void QWinRTFontDatabase::populateFontDatabase()
{
    qCDebug(lcQpaFonts) << __FUNCTION__;

    ComPtr<IDWriteFactory1> factory;
    HRESULT hr = DWriteCreateFactory(DWRITE_FACTORY_TYPE_ISOLATED, __uuidof(IDWriteFactory1), &factory);
    if (FAILED(hr)) {
        qWarning("Failed to create DirectWrite factory: %s", qPrintable(qt_error_string(hr)));
        QFreeTypeFontDatabase::populateFontDatabase();
        return;
    }

    ComPtr<IDWriteFontCollection> fontCollection;
    hr = factory->GetSystemFontCollection(&fontCollection);
    if (FAILED(hr)) {
        qWarning("Failed to open system font collection: %s", qPrintable(qt_error_string(hr)));
        QFreeTypeFontDatabase::populateFontDatabase();
        return;
    }

    int fontFamilyCount = fontCollection->GetFontFamilyCount();
    for (int i = 0; i < fontFamilyCount; ++i) {
        ComPtr<IDWriteFontFamily> fontFamily;
        hr = fontCollection->GetFontFamily(i, &fontFamily);
        if (FAILED(hr)) {
            qWarning("Unable to get font family: %s", qPrintable(qt_error_string(hr)));
            continue;
        }

        ComPtr<IDWriteLocalizedStrings> names;
        hr = fontFamily->GetFamilyNames(&names);
        if (FAILED(hr)) {
            qWarning("Unable to get font family names: %s", qPrintable(qt_error_string(hr)));
            continue;
        }
        quint32 familyNameLength;
        hr = names->GetStringLength(0, &familyNameLength);
        if (FAILED(hr)) {
            qWarning("Unable to get family name length: %s", qPrintable(qt_error_string(hr)));
            continue;
        }
        QVector<wchar_t> familyBuffer(familyNameLength + 1);
        hr = names->GetString(0, familyBuffer.data(), familyBuffer.size());
        if (FAILED(hr)) {
            qWarning("Unable to create font family name: %s", qPrintable(qt_error_string(hr)));
            continue;
        }
        QString familyName = QString::fromWCharArray(familyBuffer.data(), familyNameLength);

        m_fontFamilies.insert(familyName, fontFamily.Detach());

        registerFontFamily(familyName);
    }

    QFreeTypeFontDatabase::populateFontDatabase();
}

void QWinRTFontDatabase::populateFamily(const QString &familyName)
{
    qCDebug(lcQpaFonts) << __FUNCTION__ << familyName;

    IDWriteFontFamily *fontFamily = m_fontFamilies.value(familyName);
    if (!fontFamily) {
        qWarning("The font family %s was not found.", qPrintable(familyName));
        return;
    }

    bool fontRegistered = false;
    const int fontCount = fontFamily->GetFontCount();
    for (int j = 0; j < fontCount; ++j) {
        ComPtr<IDWriteFont> font;
        HRESULT hr = fontFamily->GetFont(j, &font);
        if (FAILED(hr)) {
            qWarning("Unable to get font: %s", qPrintable(qt_error_string(hr)));
            continue;
        }

        // Skip simulated faces
        if (font->GetSimulations() != DWRITE_FONT_SIMULATIONS_NONE)
            continue;

        ComPtr<IDWriteFontFace> baseFontFace;
        hr = font->CreateFontFace(&baseFontFace);
        if (FAILED(hr)) {
            qWarning("Unable to create base font face: %s", qPrintable(qt_error_string(hr)));
            continue;
        }
        ComPtr<IDWriteFontFace1> fontFace;
        hr = baseFontFace.As(&fontFace);
        if (FAILED(hr)) {
            qWarning("Unable to create font face: %s", qPrintable(qt_error_string(hr)));
            continue;
        }

        // We can't deal with multi-file fonts
        quint32 fileCount;
        hr = fontFace->GetFiles(&fileCount, NULL);
        if (FAILED(hr)) {
            qWarning("Unable to get font file count: %s", qPrintable(qt_error_string(hr)));
            continue;
        }
        if (fileCount != 1)
            continue;

        ComPtr<IDWriteLocalizedStrings> informationalStrings;
        BOOL exists;
        hr = font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_MANUFACTURER,
                                           &informationalStrings, &exists);
        if (FAILED(hr)) {
            qWarning("Unable to get font foundry: %s", qPrintable(qt_error_string(hr)));
            continue;
        }
        QString foundryName;
        if (exists) {
            quint32 length;
            hr = informationalStrings->GetStringLength(0, &length);
            if (FAILED(hr))
                qWarning("Unable to get foundry name length: %s", qPrintable(qt_error_string(hr)));
            if (SUCCEEDED(hr)) {
                QVector<wchar_t> buffer(length + 1);
                hr = informationalStrings->GetString(0, buffer.data(), buffer.size());
                if (FAILED(hr))
                    qWarning("Unable to get foundry name: %s", qPrintable(qt_error_string(hr)));
                if (SUCCEEDED(hr))
                    foundryName = QString::fromWCharArray(buffer.data(), length);
            }
        }

        QFont::Weight weight = QPlatformFontDatabase::weightFromInteger(font->GetWeight());

        QFont::Style style;
        switch (font->GetStyle()) {
        default:
        case DWRITE_FONT_STYLE_NORMAL:
            style = QFont::StyleNormal;
            break;
        case DWRITE_FONT_STYLE_OBLIQUE:
            style = QFont::StyleOblique;
            break;
        case DWRITE_FONT_STYLE_ITALIC:
            style = QFont::StyleItalic;
            break;
        }

        QFont::Stretch stretch;
        switch (font->GetStretch()) {
        default:
        case DWRITE_FONT_STRETCH_UNDEFINED:
        case DWRITE_FONT_STRETCH_NORMAL:
            stretch = QFont::Unstretched;
            break;
        case DWRITE_FONT_STRETCH_ULTRA_CONDENSED:
            stretch = QFont::UltraCondensed;
            break;
        case DWRITE_FONT_STRETCH_EXTRA_CONDENSED:
            stretch = QFont::ExtraCondensed;
            break;
        case DWRITE_FONT_STRETCH_CONDENSED:
            stretch = QFont::Condensed;
            break;
        case DWRITE_FONT_STRETCH_SEMI_CONDENSED:
            stretch = QFont::SemiCondensed;
            break;
        case DWRITE_FONT_STRETCH_SEMI_EXPANDED:
            stretch = QFont::SemiExpanded;
            break;
        case DWRITE_FONT_STRETCH_EXPANDED:
            stretch = QFont::Expanded;
            break;
        case DWRITE_FONT_STRETCH_EXTRA_EXPANDED:
            stretch = QFont::ExtraExpanded;
            break;
        case DWRITE_FONT_STRETCH_ULTRA_EXPANDED:
            stretch = QFont::UltraExpanded;
            break;
        }

        const bool fixedPitch = fontFace->IsMonospacedFont();

        // Get writing systems from unicode ranges
        quint32 actualRangeCount;
        hr = fontFace->GetUnicodeRanges(0, nullptr, &actualRangeCount);
        Q_ASSERT(hr == E_NOT_SUFFICIENT_BUFFER);
        QVector<DWRITE_UNICODE_RANGE> unicodeRanges(actualRangeCount);
        hr = fontFace->GetUnicodeRanges(actualRangeCount, unicodeRanges.data(), &actualRangeCount);
        if (FAILED(hr)) {
            qWarning("Unable to get font unicode range: %s", qPrintable(qt_error_string(hr)));
            continue;
        }
        QSupportedWritingSystems writingSystems;
        for (quint32 i = 0; i < actualRangeCount; ++i) {
            const QFontDatabase::WritingSystem writingSystem = writingSystemFromUnicodeRange(unicodeRanges.at(i));
            writingSystems.setSupported(writingSystem);
        }
        if (writingSystems.supported(QFontDatabase::SimplifiedChinese)) {
            writingSystems.setSupported(QFontDatabase::TraditionalChinese);
            writingSystems.setSupported(QFontDatabase::Japanese);
        }
        if (writingSystems.supported(QFontDatabase::Latin))
            writingSystems.setSupported(QFontDatabase::Vietnamese);

        IDWriteFontFile *fontFile;
        hr = fontFace->GetFiles(&fileCount, &fontFile);
        if (FAILED(hr)) {
            qWarning("Unable to get font file: %s", qPrintable(qt_error_string(hr)));
            continue;
        }

        FontDescription description = { fontFace->GetIndex(), QUuid::createUuid().toByteArray() };
        m_fonts.insert(fontFile, description);
        registerFont(familyName, QString(), foundryName, weight, style, stretch,
                     true, true, 0, fixedPitch, writingSystems, fontFile);
        fontRegistered = true;
    }

    // Always populate something to avoid an assert
    if (!fontRegistered) {
        registerFont(familyName, QString(), QString(), QFont::Normal, QFont::StyleNormal,
                     QFont::Unstretched, false, false, 0, false, QSupportedWritingSystems(), 0);
    }
}

QFontEngine *QWinRTFontDatabase::fontEngine(const QFontDef &fontDef, void *handle)
{
    qCDebug(lcQpaFonts) << __FUNCTION__ << "FONTDEF" << fontDef << handle;

    if (!handle) // Happens if a font family population failed
        return 0;

    IDWriteFontFile *fontFile = reinterpret_cast<IDWriteFontFile *>(handle);
    if (!m_fonts.contains(fontFile))
        return QFreeTypeFontDatabase::fontEngine(fontDef, handle);

    const void *referenceKey;
    quint32 referenceKeySize;
    HRESULT hr = fontFile->GetReferenceKey(&referenceKey, &referenceKeySize);
    if (FAILED(hr)) {
        qWarning("Unable to get font file reference key: %s", qPrintable(qt_error_string(hr)));
        return 0;
    }

    ComPtr<IDWriteFontFileLoader> loader;
    hr = fontFile->GetLoader(&loader);
    if (FAILED(hr)) {
        qWarning("Unable to get font file loader: %s", qPrintable(qt_error_string(hr)));
        return 0;
    }

    ComPtr<IDWriteFontFileStream> stream;
    hr =loader->CreateStreamFromKey(referenceKey, referenceKeySize, &stream);
    if (FAILED(hr)) {
        qWarning("Unable to get font file stream: %s", qPrintable(qt_error_string(hr)));
        return 0;
    }

    quint64 fileSize;
    hr = stream->GetFileSize(&fileSize);
    if (FAILED(hr)) {
        qWarning("Unable to get font file size: %s", qPrintable(qt_error_string(hr)));
        return 0;
    }

    const void *data;
    void *context;
    hr = stream->ReadFileFragment(&data, 0, fileSize, &context);
    if (FAILED(hr)) {
        qWarning("Unable to get font file data: %s", qPrintable(qt_error_string(hr)));
        return 0;
    }
    const QByteArray fontData((const char *)data, fileSize);
    stream->ReleaseFileFragment(context);

    QFontEngine::FaceId faceId;
    const FontDescription description = m_fonts.value(fontFile);
    faceId.uuid = description.uuid;
    faceId.index = description.index;

    return QFontEngineFT::create(fontDef, faceId, fontData);
}

QString QWinRTFontDatabase::familyForStyleHint(QFont::StyleHint styleHint)
{
    switch (styleHint) {
    case QFont::Times:
        return QStringLiteral("Times New Roman");
    case QFont::Courier:
        return QStringLiteral("Courier New");
    case QFont::Monospace:
        return QStringLiteral("Courier New");
    case QFont::Cursive:
        return QStringLiteral("Comic Sans MS");
    case QFont::Fantasy:
        return QStringLiteral("Impact");
    case QFont::Decorative:
        return QStringLiteral("Old English");
    case QFont::Helvetica:
        return QStringLiteral("Segoe UI");
    case QFont::System:
    default:
        break;
    }
    return QStringLiteral("Segoe UI");
}

QStringList QWinRTFontDatabase::fallbacksForFamily(const QString &family, QFont::Style style,
                                                   QFont::StyleHint styleHint,
                                                   QChar::Script script) const
{
    Q_UNUSED(style)
    Q_UNUSED(script)

    qCDebug(lcQpaFonts) << __FUNCTION__ << family;

    QStringList result;
    result.append(QWinRTFontDatabase::familyForStyleHint(styleHint));
    result.append(QFreeTypeFontDatabase::fallbacksForFamily(family, style, styleHint, script));
    return result;
}

void QWinRTFontDatabase::releaseHandle(void *handle)
{
    qCDebug(lcQpaFonts) << __FUNCTION__ << handle;

    if (!handle)
        return;

    IDWriteFontFile *fontFile = reinterpret_cast<IDWriteFontFile *>(handle);
    if (m_fonts.contains(fontFile)) {
        m_fonts.remove(fontFile);
        fontFile->Release();
        return;
    }

    QFreeTypeFontDatabase::releaseHandle(handle);
}

QT_END_NAMESPACE
