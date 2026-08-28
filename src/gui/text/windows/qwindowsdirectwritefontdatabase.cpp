// Copyright (C) 2020 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsdirectwritefontdatabase_p.h"
#include "qwindowsfontenginedirectwrite_p.h"
#include "qwindowsfontdatabase_p.h"

#include <QtCore/qendian.h>
#include <QtCore/qfile.h>
#include <QtCore/qstringbuilder.h>
#include <QtCore/qvarlengtharray.h>

#include <dwrite_3.h>
#include <d2d1.h>

QT_BEGIN_NAMESPACE

// Defined in gui/text/qfontdatabase.cpp
Q_GUI_EXPORT QFontDatabase::WritingSystem qt_writing_system_for_script(int script);

template<typename T>
struct DirectWriteScope {
    DirectWriteScope(T *res = nullptr) : m_res(res) {}
    ~DirectWriteScope() {
        if (m_res != nullptr)
            m_res->Release();
    }

    T **operator&()
    {
        return &m_res;
    }

    T *operator->()
    {
        return m_res;
    }

    T *operator*() {
        return m_res;
    }

private:
    T *m_res;
};

QWindowsDirectWriteFontDatabase::QWindowsDirectWriteFontDatabase()
{
    qCDebug(lcQpaFonts) << "Creating DirectWrite database";
}

QWindowsDirectWriteFontDatabase::~QWindowsDirectWriteFontDatabase()
{
    for (auto it = m_populatedFonts.begin(); it != m_populatedFonts.end(); ++it)
        it.value()->Release();
}

QString QWindowsDirectWriteFontDatabase::localeString(IDWriteLocalizedStrings *names,
                                                      wchar_t localeName[])
{
    uint index;
    BOOL exists;
    if (SUCCEEDED(names->FindLocaleName(localeName, &index, &exists)) && exists) {
        uint length;
        if (SUCCEEDED(names->GetStringLength(index, &length)) && length > 0) {
            QVarLengthArray<wchar_t> buffer(int(length) + 1);
            if (SUCCEEDED(names->GetString(index, buffer.data(), length + 1)))
                return QString::fromWCharArray(buffer.data());
        }
    }

    return QString();
}

QFont::Stretch QWindowsDirectWriteFontDatabase::fromDirectWriteStretch(DWRITE_FONT_STRETCH stretch)
{
    switch (stretch) {
    case DWRITE_FONT_STRETCH_ULTRA_CONDENSED: return QFont::UltraCondensed;
    case DWRITE_FONT_STRETCH_EXTRA_CONDENSED: return QFont::ExtraCondensed;
    case DWRITE_FONT_STRETCH_CONDENSED: return QFont::Condensed;
    case DWRITE_FONT_STRETCH_SEMI_CONDENSED: return QFont::SemiCondensed;
    case DWRITE_FONT_STRETCH_NORMAL: return QFont::Unstretched;
    case DWRITE_FONT_STRETCH_SEMI_EXPANDED: return QFont::SemiExpanded;
    case DWRITE_FONT_STRETCH_EXPANDED: return QFont::Expanded;
    case DWRITE_FONT_STRETCH_EXTRA_EXPANDED: return QFont::ExtraExpanded;
    case DWRITE_FONT_STRETCH_ULTRA_EXPANDED: return QFont::UltraExpanded;
    default: return QFont::AnyStretch;
    }
}

QFont::Weight QWindowsDirectWriteFontDatabase::fromDirectWriteWeight(DWRITE_FONT_WEIGHT weight)
{
    return static_cast<QFont::Weight>(weight);
}

QFont::Style QWindowsDirectWriteFontDatabase::fromDirectWriteStyle(DWRITE_FONT_STYLE style)
{
    switch (style) {
    case DWRITE_FONT_STYLE_NORMAL: return QFont::StyleNormal;
    case DWRITE_FONT_STYLE_OBLIQUE: return QFont::StyleOblique;
    case DWRITE_FONT_STYLE_ITALIC: return QFont::StyleItalic;
    default: return QFont::StyleNormal;
    }
}

void QWindowsDirectWriteFontDatabase::populateFamily(const QString &familyName)
{
    auto it = m_populatedFonts.find(familyName);
    if (it == m_populatedFonts.end() && m_populatedBitmapFonts.contains(familyName)) {
        qCDebug(lcQpaFonts) << "Populating bitmap font" << familyName;
        QWindowsFontDatabase::populateFamily(familyName);
        return;
    }

    IDWriteFontFamily *fontFamily = it != m_populatedFonts.end() ? it.value() : nullptr;
    if (fontFamily == nullptr) {
        qCWarning(lcQpaFonts) << "Cannot find" << familyName << "in list of fonts";
        return;
    }

    qCDebug(lcQpaFonts) << "Populate family:" << familyName;

    wchar_t defaultLocale[LOCALE_NAME_MAX_LENGTH];
    bool hasDefaultLocale = GetUserDefaultLocaleName(defaultLocale, LOCALE_NAME_MAX_LENGTH) != 0;
    wchar_t englishLocale[] = L"en-us";

    static const int SMOOTH_SCALABLE = 0xffff;
    const QString foundryName; // No such concept.
    const bool scalable = true;
    const bool antialias = false;
    const int size = SMOOTH_SCALABLE;

    DirectWriteScope<IDWriteFontList> matchingFonts;
    if (SUCCEEDED(fontFamily->GetMatchingFonts(DWRITE_FONT_WEIGHT_REGULAR,
                                               DWRITE_FONT_STRETCH_NORMAL,
                                               DWRITE_FONT_STYLE_NORMAL,
                                               &matchingFonts))) {
        for (uint j = 0; j < matchingFonts->GetFontCount(); ++j) {
            DirectWriteScope<IDWriteFont> font;
            if (SUCCEEDED(matchingFonts->GetFont(j, &font))) {
                DirectWriteScope<IDWriteFont2> font2;
                if (!SUCCEEDED(font->QueryInterface(__uuidof(IDWriteFont2),
                                                   reinterpret_cast<void **>(&font2)))) {
                    qCWarning(lcQpaFonts) << "COM object does not support IDWriteFont1";
                    continue;
                }

                QString defaultLocaleFamilyName;
                QString englishLocaleFamilyName;
                QString defaultLocaleGdiCompatibleFamilyName;
                QString englishLocaleGdiCompatibleFamilyName;

                DirectWriteScope<IDWriteFontFamily> fontFamily2;
                if (SUCCEEDED(font2->GetFontFamily(&fontFamily2))) {
                    DirectWriteScope<IDWriteLocalizedStrings> names;
                    if (SUCCEEDED(fontFamily2->GetFamilyNames(&names))) {
                        defaultLocaleFamilyName = hasDefaultLocale ? localeString(*names, defaultLocale) : QString();
                        englishLocaleFamilyName = localeString(*names, englishLocale);
                    }

                    BOOL ok;
                    if (SUCCEEDED(font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES, &names, &ok)) && ok) {
                        defaultLocaleGdiCompatibleFamilyName = hasDefaultLocale ? localeString(*names, defaultLocale) : QString();
                        englishLocaleGdiCompatibleFamilyName = localeString(*names, englishLocale);
                    }
                }

                if (defaultLocaleFamilyName.isEmpty()
                    && englishLocaleFamilyName.isEmpty()
                    && englishLocaleGdiCompatibleFamilyName.isEmpty()
                    && defaultLocaleGdiCompatibleFamilyName.isEmpty()) {
                    englishLocaleFamilyName = familyName;
                }

                QFont::Stretch stretch = fromDirectWriteStretch(font2->GetStretch());
                QFont::Style style = fromDirectWriteStyle(font2->GetStyle());
                QFont::Weight weight = fromDirectWriteWeight(font2->GetWeight());
                bool fixed = font2->IsMonospacedFont();
                bool color = font2->IsColorFont();

                DirectWriteScope<IDWriteFontFace> face;
                if (SUCCEEDED(font->CreateFontFace(&face))) {
                    QSupportedWritingSystems writingSystems = supportedWritingSystems(*face);

                    auto registerFontName = [&](const QString &rFamilyName,
                                                const QString &rStyleName) {
                        qCDebug(lcQpaFonts) << "Family" << familyName << "has variant"
                                            << rFamilyName << ", "
                                            << rStyleName << ", "
                                            << stretch << ", "
                                            << style << ", "
                                            << weight << ", "
                                            << fixed << ", "
                                            << writingSystems;
                        QPlatformFontDatabase::registerFont(rFamilyName,
                                                            rStyleName,
                                                            QString(),
                                                            weight,
                                                            style,
                                                            stretch,
                                                            antialias,
                                                            scalable,
                                                            size,
                                                            fixed,
                                                            color,
                                                            writingSystems,
                                                            new FontHandle(*face, rFamilyName));
                    };

                    // Register the standard face names
                    DirectWriteScope<IDWriteLocalizedStrings> names;
                    if (SUCCEEDED(font2->GetFaceNames(&names))) {
                        QString defaultLocaleStyleName = hasDefaultLocale ? localeString(*names, defaultLocale) : QString();
                        QString englishLocaleStyleName = localeString(*names, englishLocale);
                        qCDebug(lcQpaFonts) << ">>> Storing English locale names";
                        registerFontName(englishLocaleFamilyName, englishLocaleStyleName);

                        // If the font has another name in the current default locale, register
                        // this as well
                        if (!defaultLocaleFamilyName.isEmpty()
                            && defaultLocaleFamilyName != englishLocaleFamilyName) {
                            qCDebug(lcQpaFonts) << ">>> Storing default locale names";
                            registerFontName(defaultLocaleFamilyName, defaultLocaleStyleName);
                        }
                    }

                    // Register GDI compatible names if they differ from the main face names
                    if (!defaultLocaleGdiCompatibleFamilyName.isEmpty()
                        || !englishLocaleGdiCompatibleFamilyName.isEmpty()) {
                        QString defaultLocaleGdiCompatibleStyleName;
                        QString englishLocaleGdiCompatibleStyleName;

                        DirectWriteScope<IDWriteLocalizedStrings> names;
                        BOOL ok;
                        if (SUCCEEDED(font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_WIN32_SUBFAMILY_NAMES, &names, &ok)) && ok) {
                            defaultLocaleGdiCompatibleStyleName = hasDefaultLocale ? localeString(*names, defaultLocale) : QString();
                            englishLocaleGdiCompatibleStyleName = localeString(*names, englishLocale);

                            if (!englishLocaleGdiCompatibleFamilyName.isEmpty()
                                && englishLocaleGdiCompatibleFamilyName != englishLocaleFamilyName) {
                                qCDebug(lcQpaFonts) << ">>> Storing English GDI compatible names";
                                registerFontName(englishLocaleGdiCompatibleFamilyName,
                                                 englishLocaleGdiCompatibleStyleName);
                            }

                            if (!defaultLocaleGdiCompatibleFamilyName.isEmpty()
                                && defaultLocaleGdiCompatibleFamilyName != englishLocaleGdiCompatibleFamilyName
                                && defaultLocaleGdiCompatibleFamilyName != defaultLocaleFamilyName) {
                                qCDebug(lcQpaFonts) << ">>> Storing Default locale GDI compatible names";
                                registerFontName(defaultLocaleGdiCompatibleFamilyName,
                                                 defaultLocaleGdiCompatibleStyleName);
                            }
                        }
                    }
                }
            }
        }
    }
}

QSupportedWritingSystems QWindowsDirectWriteFontDatabase::supportedWritingSystems(IDWriteFontFace *face) const
{
    QSupportedWritingSystems writingSystems;
    writingSystems.setSupported(QFontDatabase::Any);

    DirectWriteScope<IDWriteFontFace1> face1;
    if (SUCCEEDED(face->QueryInterface(__uuidof(IDWriteFontFace1),
                                       reinterpret_cast<void **>(&face1)))) {
        const void *tableData = nullptr;
        UINT32 tableSize;
        void *tableContext = nullptr;
        BOOL exists;
        HRESULT hr = face->TryGetFontTable(qFromBigEndian(QFont::Tag("OS/2").value()),
                                           &tableData,
                                           &tableSize,
                                           &tableContext,
                                           &exists);
        if (SUCCEEDED(hr) && exists) {
            writingSystems = QPlatformFontDatabase::writingSystemsFromOS2Table(reinterpret_cast<const char *>(tableData), tableSize);
        } else { // Fall back to checking first character of each Unicode range in font (may include too many writing systems)
            quint32 rangeCount;
            hr = face1->GetUnicodeRanges(0, nullptr, &rangeCount);

            if (rangeCount > 0) {
                QVarLengthArray<DWRITE_UNICODE_RANGE, QChar::ScriptCount> ranges(rangeCount);

                hr = face1->GetUnicodeRanges(rangeCount, ranges.data(), &rangeCount);
                if (SUCCEEDED(hr)) {
                    for (uint i = 0; i < rangeCount; ++i) {
                        QChar::Script script = QChar::script(ranges.at(i).first);

                        QFontDatabase::WritingSystem writingSystem = qt_writing_system_for_script(script);

                        if (writingSystem > QFontDatabase::Any && writingSystem < QFontDatabase::WritingSystemsCount)
                            writingSystems.setSupported(writingSystem);
                    }
                } else {
                    const QString errorString = qt_error_string(int(hr));
                    qCWarning(lcQpaFonts) << "Failed to get unicode ranges for font:" << errorString;
                }
            }
        }
    }

    return writingSystems;
}

bool QWindowsDirectWriteFontDatabase::populateFamilyAliases(const QString &missingFamily)
{
    // Skip over implementation in QWindowsFontDatabase
    return QWindowsFontDatabaseBase::populateFamilyAliases(missingFamily);
}

QFontEngine *QWindowsDirectWriteFontDatabase::fontEngine(const QByteArray &fontData,
                                                         qreal pixelSize,
                                                         QFont::HintingPreference hintingPreference)
{
    // Skip over implementation in QWindowsFontDatabase
    return QWindowsFontDatabaseBase::fontEngine(fontData, pixelSize, hintingPreference);
}

QFontEngine *QWindowsDirectWriteFontDatabase::fontEngine(const QFontDef &fontDef, void *handle)
{
    const FontHandle *fontHandle = static_cast<const FontHandle *>(handle);
    IDWriteFontFace *face = fontHandle->fontFace;
    if (face == nullptr) {
        qCDebug(lcQpaFonts) << "Falling back to GDI";
        return QWindowsFontDatabase::fontEngine(fontDef, handle);
    }

    DWRITE_FONT_SIMULATIONS simulations = DWRITE_FONT_SIMULATIONS_NONE;
    if (fontDef.weight >= QFont::DemiBold || fontDef.style != QFont::StyleNormal) {
        DirectWriteScope<IDWriteFontFace3> face3;
        if (SUCCEEDED(face->QueryInterface(__uuidof(IDWriteFontFace3),
                                           reinterpret_cast<void **>(&face3)))) {
            if (fontDef.weight >= QFont::DemiBold && face3->GetWeight() < DWRITE_FONT_WEIGHT_DEMI_BOLD)
                simulations |= DWRITE_FONT_SIMULATIONS_BOLD;

            if (fontDef.style != QFont::StyleNormal && face3->GetStyle() == DWRITE_FONT_STYLE_NORMAL)
                simulations |= DWRITE_FONT_SIMULATIONS_OBLIQUE;
        }
    }

    DirectWriteScope<IDWriteFontFace5> newFace;
    if (!fontDef.variableAxisValues.isEmpty() || simulations != DWRITE_FONT_SIMULATIONS_NONE) {
        DirectWriteScope<IDWriteFontFace5> face5;
        if (SUCCEEDED(face->QueryInterface(__uuidof(IDWriteFontFace5),
                                           reinterpret_cast<void **>(&face5)))) {
            DirectWriteScope<IDWriteFontResource> font;
            if (SUCCEEDED(face5->GetFontResource(&font))) {
                UINT32 fontAxisCount = font->GetFontAxisCount();
                QVarLengthArray<DWRITE_FONT_AXIS_VALUE, 8> fontAxisValues(fontAxisCount);

                if (!fontDef.variableAxisValues.isEmpty()) {
                    if (SUCCEEDED(face5->GetFontAxisValues(fontAxisValues.data(), fontAxisCount))) {
                        for (UINT32 i = 0; i < fontAxisCount; ++i) {
                            if (auto maybeTag = QFont::Tag::fromValue(qToBigEndian<UINT32>(fontAxisValues[i].axisTag))) {
                                if (fontDef.variableAxisValues.contains(*maybeTag))
                                    fontAxisValues[i].value = fontDef.variableAxisValues.value(*maybeTag);
                            }
                        }
                    }
                }

                if (SUCCEEDED(font->CreateFontFace(simulations,
                                                   !fontDef.variableAxisValues.isEmpty() ? fontAxisValues.data() : nullptr,
                                                   !fontDef.variableAxisValues.isEmpty() ? fontAxisCount : 0,
                                                   &newFace))) {
                    face = *newFace;
                } else {
                    qCWarning(lcQpaFonts) << "DirectWrite: Can't create font face for variable axis values";
                }
            }
        }
    }

    QWindowsFontEngineDirectWrite *fontEngine = new QWindowsFontEngineDirectWrite(face, fontDef.pixelSize, data());
    fontEngine->initFontInfo(fontDef, defaultVerticalDPI());

    return fontEngine;
}

QStringList QWindowsDirectWriteFontDatabase::fallbacksForFamily(const QString &family,
                                                                QFont::Style style,
                                                                QFont::StyleHint styleHint,
                                                                QFontDatabasePrivate::ExtendedScript script) const
{
    QStringList result;
    result.append(QWindowsFontDatabaseBase::familiesForScript(script));
    result.append(familyForStyleHint(styleHint));
    result.append(extraTryFontsForFamily(family));
    result.append(QPlatformFontDatabase::fallbacksForFamily(family, style, styleHint, script));

    qCDebug(lcQpaFonts) << __FUNCTION__ << family << style << styleHint
        << script << result;
    return result;
}

template<typename T>
void QWindowsDirectWriteFontDatabase::collectAdditionalNames(T *font,
                                                             wchar_t *defaultLocale,
                                                             wchar_t *englishLocale,
                                                             std::function<void(const QString &, const QString &)> registerFamily)
{
    BOOL ok;
    QString defaultLocaleGdiCompatibleFamilyName;
    QString englishLocaleGdiCompatibleFamilyName;

    const bool hasDefaultLocale = defaultLocale != nullptr;
    Q_ASSERT(englishLocale != nullptr);

    IDWriteLocalizedStrings *names = nullptr;
    if (SUCCEEDED(font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_WIN32_FAMILY_NAMES, &names, &ok)) && ok) {
        defaultLocaleGdiCompatibleFamilyName = hasDefaultLocale ? localeString(names, defaultLocale) : QString();
        englishLocaleGdiCompatibleFamilyName = localeString(names, englishLocale);

        names->Release();
    }

    QString defaultLocaleGdiCompatibleStyleName;
    QString englishLocaleGdiCompatibleStyleName;
    if (SUCCEEDED(font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_WIN32_SUBFAMILY_NAMES, &names, &ok)) && ok) {
        defaultLocaleGdiCompatibleStyleName = hasDefaultLocale ? localeString(names, defaultLocale) : QString();
        englishLocaleGdiCompatibleStyleName = localeString(names, englishLocale);

        names->Release();
    }

    QString defaultLocaleTypographicFamilyName;
    QString englishLocaleTypographicFamilyName;
    if (SUCCEEDED(font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_TYPOGRAPHIC_FAMILY_NAMES, &names, &ok)) && ok) {
        defaultLocaleTypographicFamilyName = hasDefaultLocale ? localeString(names, defaultLocale) : QString();
        englishLocaleTypographicFamilyName = localeString(names, englishLocale);

        names->Release();
    }

    QString defaultLocaleTypographicStyleName;
    QString englishLocaleTypographicStyleName;
    if (SUCCEEDED(font->GetInformationalStrings(DWRITE_INFORMATIONAL_STRING_TYPOGRAPHIC_SUBFAMILY_NAMES, &names, &ok)) && ok) {
        defaultLocaleTypographicStyleName = hasDefaultLocale ? localeString(names, defaultLocale) : QString();
        englishLocaleTypographicStyleName = localeString(names, englishLocale);

        names->Release();
    }

    if (!englishLocaleGdiCompatibleFamilyName.isEmpty())
        registerFamily(englishLocaleGdiCompatibleFamilyName, englishLocaleGdiCompatibleStyleName);

    if (!defaultLocaleGdiCompatibleFamilyName.isEmpty())
        registerFamily(defaultLocaleGdiCompatibleFamilyName, defaultLocaleGdiCompatibleStyleName);

    if (!englishLocaleTypographicFamilyName.isEmpty())
        registerFamily(englishLocaleTypographicFamilyName, englishLocaleTypographicStyleName);

    if (!defaultLocaleTypographicFamilyName.isEmpty())
        registerFamily(defaultLocaleTypographicFamilyName, defaultLocaleTypographicStyleName);
}

namespace {
    struct FontInfo {
        QFont::Stretch stretch;
        QFont::Style style;
        QFont::Weight weight;
        QString familyName;
        QString styleName;

        bool operator==(const FontInfo &other) const
        {
            return stretch == other.stretch
                       && style == other.style
                       && weight == other.weight
                       && familyName == other.familyName
                       && styleName == other.styleName;
        }

        friend size_t qHash(const FontInfo &key, size_t seed = 0) noexcept
        {
            return qHashMulti(seed,
                              key.stretch,
                              key.style,
                              key.weight,
                              key.familyName,
                              key.styleName);
        }
    };

}

QStringList QWindowsDirectWriteFontDatabase::addApplicationFont(const QByteArray &fontData, const QString &fileName, QFontDatabasePrivate::ApplicationFont *applicationFont)
{
    qCDebug(lcQpaFonts) << "Adding application font" << fileName;

    QByteArray loadedData = fontData;
    if (loadedData.isEmpty()) {
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            qCWarning(lcQpaFonts) << "Cannot open" << fileName << "for reading.";
            return QStringList();
        }
        loadedData = file.readAll();
    }

    QList<IDWriteFontFace *> faces = createDirectWriteFaces(loadedData, fileName);
    if (faces.isEmpty()) {
        qCWarning(lcQpaFonts) << "Failed to create DirectWrite face from font data. Font may be unsupported.";
        return QStringList();
    }

    QSet<FontInfo> registeredFonts;
    QSet<QString> ret;
    for (int i = 0; i < faces.size(); ++i) {
        IDWriteFontFace *face = faces.at(i);
        wchar_t defaultLocale[LOCALE_NAME_MAX_LENGTH];
        bool hasDefaultLocale = GetUserDefaultLocaleName(defaultLocale, LOCALE_NAME_MAX_LENGTH) != 0;
        wchar_t englishLocale[] = L"en-us";

        static const int SMOOTH_SCALABLE = 0xffff;
        const bool scalable = true;
        const bool antialias = false;
        const int size = SMOOTH_SCALABLE;

        QSupportedWritingSystems writingSystems = supportedWritingSystems(face);
        DirectWriteScope<IDWriteFontFace3> face3;
        if (SUCCEEDED(face->QueryInterface(__uuidof(IDWriteFontFace3),
                                           reinterpret_cast<void **>(&face3)))) {

            QFont::Stretch stretch = fromDirectWriteStretch(face3->GetStretch());
            QFont::Style style = fromDirectWriteStyle(face3->GetStyle());
            QFont::Weight weight = fromDirectWriteWeight(face3->GetWeight());
            bool fixed = face3->IsMonospacedFont();
            bool color = face3->IsColorFont();

            auto registerFamilyAndStyle = [&registeredFonts,
                                           &ret,
                                           applicationFont,
                                           face,
                                           writingSystems,
                                           stretch,
                                           style,
                                           weight,
                                           fixed,
                                           size,
                                           antialias,
                                           scalable,
                                           color](const QString &familyName,
                                                  const QString &styleName)
            {
                FontInfo fontInfo = { stretch, style, weight, familyName, styleName };
                if (registeredFonts.contains(fontInfo))
                    return;
                registeredFonts.insert(fontInfo);
                ret.insert(familyName);

                qCDebug(lcQpaFonts) << "\tRegistering alternative:" << familyName
                                    << ":" << styleName
                                    << ", stretch =" << stretch
                                    << ", style =" << style
                                    << ", weight =" << weight
                                    << ", fixed =" << fixed
                                    << ", color =" << color;
                if (applicationFont != nullptr) {
                    QFontDatabasePrivate::ApplicationFont::Properties properties;
                    properties.style = style;
                    properties.weight = weight;
                    properties.familyName = familyName;
                    properties.styleName = styleName;
                    properties.stretch = stretch;
                    applicationFont->properties.append(properties);
                }

                QPlatformFontDatabase::registerFont(familyName,
                                                    styleName,
                                                    QString(),
                                                    weight,
                                                    style,
                                                    stretch,
                                                    antialias,
                                                    scalable,
                                                    size,
                                                    fixed,
                                                    color,
                                                    writingSystems,
                                                    new FontHandle(face, familyName));
            };

            QString defaultLocaleFamilyName;
            QString englishLocaleFamilyName;

            IDWriteLocalizedStrings *names = nullptr;
            if (SUCCEEDED(face3->GetFamilyNames(&names))) {
                defaultLocaleFamilyName = hasDefaultLocale ? localeString(names, defaultLocale) : QString();
                englishLocaleFamilyName = localeString(names, englishLocale);

                names->Release();
            }

            QString defaultLocaleStyleName;
            QString englishLocaleStyleName;
            if (SUCCEEDED(face3->GetFaceNames(&names))) {
                defaultLocaleStyleName = hasDefaultLocale ? localeString(names, defaultLocale) : QString();
                englishLocaleStyleName = localeString(names, englishLocale);

                names->Release();
            }

            qCDebug(lcQpaFonts) << "\tFont names:" << englishLocaleFamilyName << ", " << defaultLocaleFamilyName
                                << ", style names:" << englishLocaleStyleName << ", " << defaultLocaleStyleName
                                << ", stretch:" << stretch
                                << ", style:" << style
                                << ", weight:" << weight
                                << ", fixed:" << fixed;

            if (!englishLocaleFamilyName.isEmpty())
                registerFamilyAndStyle(englishLocaleFamilyName, englishLocaleStyleName);

            if (!defaultLocaleFamilyName.isEmpty())
                registerFamilyAndStyle(defaultLocaleFamilyName, defaultLocaleStyleName);

            collectAdditionalNames(*face3,
                                   hasDefaultLocale ? defaultLocale : nullptr,
                                   englishLocale,
                                   registerFamilyAndStyle);

        } else {
            qCWarning(lcQpaFonts) << "Unable to query IDWriteFontFace3 interface from font face.";
        }

        face->Release();
    }

    return ret.values();
}

bool QWindowsDirectWriteFontDatabase::isPrivateFontFamily(const QString &family) const
{
    Q_UNUSED(family);
    return false;
}

static int QT_WIN_CALLBACK populateBitmapFonts(const LOGFONT *logFont,
                                               const TEXTMETRIC *textmetric,
                                               DWORD type,
                                               LPARAM lparam)
{
    Q_UNUSED(textmetric);

    // the "@family" fonts are just the same as "family". Ignore them.
    const ENUMLOGFONTEX *f = reinterpret_cast<const ENUMLOGFONTEX *>(logFont);
    const wchar_t *faceNameW = f->elfLogFont.lfFaceName;
    if (faceNameW[0] && faceNameW[0] != L'@' && wcsncmp(faceNameW, L"WST_", 4)) {
        const QString faceName = QString::fromWCharArray(faceNameW);
        if (type & RASTER_FONTTYPE || type == 0) {
            QWindowsDirectWriteFontDatabase *db = reinterpret_cast<QWindowsDirectWriteFontDatabase *>(lparam);
            if (!db->hasPopulatedFont(faceName)) {
                db->registerFontFamily(faceName);
                db->registerBitmapFont(faceName);
            }
        }
    }
    return 1; // continue
}

void QWindowsDirectWriteFontDatabase::populateFontDatabase()
{
    wchar_t defaultLocale[LOCALE_NAME_MAX_LENGTH];
    bool hasDefaultLocale = GetUserDefaultLocaleName(defaultLocale, LOCALE_NAME_MAX_LENGTH) != 0;
    wchar_t englishLocale[] = L"en-us";

    const QString defaultFontName = defaultFont().family();
    const QString systemDefaultFontName = systemDefaultFont().family();

    DirectWriteScope<IDWriteFontCollection2> fontCollection;
    DirectWriteScope<IDWriteFactory6> factory6;
    if (FAILED(data()->directWriteFactory->QueryInterface(__uuidof(IDWriteFactory6),
                                                          reinterpret_cast<void **>(&factory6)))) {
        qCWarning(lcQpaFonts) << "Can't initialize IDWriteFactory6. Use GDI font engine instead.";
        return;
    }

    if (SUCCEEDED(factory6->GetSystemFontCollection(false,
                                                    DWRITE_FONT_FAMILY_MODEL_TYPOGRAPHIC,
                                                    &fontCollection))) {
        QSet<QString> registeredFamilies;
        for (uint i = 0; i < fontCollection->GetFontFamilyCount(); ++i) {
            DirectWriteScope<IDWriteFontFamily2> fontFamily;
            if (SUCCEEDED(fontCollection->GetFontFamily(i, &fontFamily))) {
                auto registerFamily = [&](const QString &familyName, const QString &styleName) {
                    Q_UNUSED(styleName);
                    if (registeredFamilies.contains(familyName))
                        return;
                    registeredFamilies.insert(familyName);

                    qCDebug(lcQpaFonts) << "Registering font family" << familyName;
                    registerFontFamily(familyName);
                    m_populatedFonts.insert(familyName, *fontFamily);
                    fontFamily->AddRef();

                    if (familyName == defaultFontName
                        && defaultFontName != systemDefaultFontName) {
                        qCDebug(lcQpaFonts) << "Adding default font" << systemDefaultFontName
                                            << "as alternative to" << familyName;

                        m_populatedFonts.insert(systemDefaultFontName, *fontFamily);
                        fontFamily->AddRef();
                    }
                };

                QString defaultLocaleName;
                QString englishLocaleName;
                DirectWriteScope<IDWriteLocalizedStrings> names;
                if (SUCCEEDED(fontFamily->GetFamilyNames(&names))) {
                    if (hasDefaultLocale)
                        defaultLocaleName = localeString(*names, defaultLocale);
                    englishLocaleName = localeString(*names, englishLocale);
                }

                {
                    if (!defaultLocaleName.isEmpty())
                        registerFamily(defaultLocaleName, QString{});
                }

                {
                    if (!englishLocaleName.isEmpty())
                        registerFamily(englishLocaleName, QString{});
                }

                for (uint j = 0; j < fontFamily->GetFontCount(); ++j) {
                    DirectWriteScope<IDWriteFont3> font;
                    if (SUCCEEDED(fontFamily->GetFont(j, &font))) {
                        collectAdditionalNames(*font,
                                               hasDefaultLocale ? defaultLocale : nullptr,
                                               englishLocale,
                                               registerFamily);
                    }
                }
            }
        }
    }

    // Since bitmap fonts are not supported by DirectWrite, we need to populate these as well
    {
        HDC dummy = GetDC(0);
        LOGFONT lf;
        lf.lfCharSet = DEFAULT_CHARSET;
        lf.lfFaceName[0] = 0;
        lf.lfPitchAndFamily = 0;
        EnumFontFamiliesEx(dummy, &lf, populateBitmapFonts, reinterpret_cast<intptr_t>(this), 0);
        ReleaseDC(0, dummy);
    }
}

bool QWindowsDirectWriteFontDatabase::supportsVariableApplicationFonts() const
{
    QSharedPointer<QWindowsFontEngineData> fontEngineData = data();
    DirectWriteScope<IDWriteFactory5> factory5;
    if (SUCCEEDED(fontEngineData->directWriteFactory->QueryInterface(__uuidof(IDWriteFactory5),
                                                                     reinterpret_cast<void **>(&factory5)))) {
        return true;
    }

    return false;
}

void QWindowsDirectWriteFontDatabase::invalidate()
{
    QWindowsFontDatabase::invalidate();

    for (IDWriteFontFamily *value : m_populatedFonts)
        value->Release();
    m_populatedFonts.clear();
    m_populatedFonts.squeeze();

    m_populatedBitmapFonts.clear();
    m_populatedBitmapFonts.squeeze();
}

QT_END_NAMESPACE
