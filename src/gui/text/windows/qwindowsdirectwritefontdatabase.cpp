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

static QFont::Stretch fromDirectWriteStretch(DWRITE_FONT_STRETCH stretch)
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

static QFont::Weight fromDirectWriteWeight(DWRITE_FONT_WEIGHT weight)
{
    return static_cast<QFont::Weight>(weight);
}

static QFont::Style fromDirectWriteStyle(DWRITE_FONT_STYLE style)
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

    IDWriteFontList *matchingFonts;
    if (SUCCEEDED(fontFamily->GetMatchingFonts(DWRITE_FONT_WEIGHT_REGULAR,
                                               DWRITE_FONT_STRETCH_NORMAL,
                                               DWRITE_FONT_STYLE_NORMAL,
                                               &matchingFonts))) {
        for (uint j = 0; j < matchingFonts->GetFontCount(); ++j) {
            IDWriteFont *font;
            if (SUCCEEDED(matchingFonts->GetFont(j, &font))) {
                IDWriteFont1 *font1 = nullptr;
                if (!SUCCEEDED(font->QueryInterface(__uuidof(IDWriteFont1),
                                                   reinterpret_cast<void **>(&font1)))) {
                    qCWarning(lcQpaFonts) << "COM object does not support IDWriteFont1";
                    continue;
                }

                QString defaultLocaleFamilyName;
                QString englishLocaleFamilyName;

                IDWriteFontFamily *fontFamily2;
                if (SUCCEEDED(font1->GetFontFamily(&fontFamily2))) {
                    IDWriteLocalizedStrings *names;
                    if (SUCCEEDED(fontFamily2->GetFamilyNames(&names))) {
                        defaultLocaleFamilyName = hasDefaultLocale ? localeString(names, defaultLocale) : QString();
                        englishLocaleFamilyName = localeString(names, englishLocale);

                        names->Release();
                    }

                    fontFamily2->Release();
                }

                if (defaultLocaleFamilyName.isEmpty() && englishLocaleFamilyName.isEmpty())
                    englishLocaleFamilyName = familyName;

                {
                    IDWriteLocalizedStrings *names;
                    if (SUCCEEDED(font1->GetFaceNames(&names))) {
                        QString defaultLocaleStyleName = hasDefaultLocale ? localeString(names, defaultLocale) : QString();
                        QString englishLocaleStyleName = localeString(names, englishLocale);

                        QFont::Stretch stretch = fromDirectWriteStretch(font1->GetStretch());
                        QFont::Style style = fromDirectWriteStyle(font1->GetStyle());
                        QFont::Weight weight = fromDirectWriteWeight(font1->GetWeight());
                        bool fixed = font1->IsMonospacedFont();

                        qCDebug(lcQpaFonts) << "Family" << familyName << "has english variant" << englishLocaleStyleName << ", in default locale:" << defaultLocaleStyleName << stretch << style << weight << fixed;

                        IDWriteFontFace *face = nullptr;
                        if (SUCCEEDED(font->CreateFontFace(&face))) {
                            QSupportedWritingSystems writingSystems;

                            const void *tableData = nullptr;
                            UINT32 tableSize;
                            void *tableContext = nullptr;
                            BOOL exists;
                            HRESULT hr = face->TryGetFontTable(qbswap<quint32>(MAKE_TAG('O','S','/','2')),
                                                               &tableData,
                                                               &tableSize,
                                                               &tableContext,
                                                               &exists);
                            if (SUCCEEDED(hr) && exists) {
                                writingSystems = QPlatformFontDatabase::writingSystemsFromOS2Table(reinterpret_cast<const char *>(tableData), tableSize);
                            } else { // Fall back to checking first character of each Unicode range in font (may include too many writing systems)
                                quint32 rangeCount;
                                hr = font1->GetUnicodeRanges(0, nullptr, &rangeCount);

                                if (rangeCount > 0) {
                                    QVarLengthArray<DWRITE_UNICODE_RANGE, QChar::ScriptCount> ranges(rangeCount);

                                    hr = font1->GetUnicodeRanges(rangeCount, ranges.data(), &rangeCount);
                                    if (SUCCEEDED(hr)) {
                                        for (uint i = 0; i < rangeCount; ++i) {
                                            QChar::Script script = QChar::script(ranges.at(i).first);

                                            QFontDatabase::WritingSystem writingSystem = qt_writing_system_for_script(script);

                                            if (writingSystem > QFontDatabase::Any && writingSystem < QFontDatabase::WritingSystemsCount)
                                                writingSystems.setSupported(writingSystem);
                                        }
                                    } else {
                                        const QString errorString = qt_error_string(int(hr));
                                        qCWarning(lcQpaFonts) << "Failed to get unicode ranges for font" << englishLocaleFamilyName << englishLocaleStyleName << ":" << errorString;
                                    }
                                }
                            }

                            if (!englishLocaleStyleName.isEmpty() || defaultLocaleStyleName.isEmpty()) {
                                qCDebug(lcQpaFonts) << "Font" << englishLocaleFamilyName << englishLocaleStyleName << "supports writing systems:" << writingSystems;

                                QPlatformFontDatabase::registerFont(englishLocaleFamilyName, englishLocaleStyleName, QString(), weight, style, stretch, antialias, scalable, size, fixed, writingSystems, face);
                                face->AddRef();
                            }

                            if (!defaultLocaleFamilyName.isEmpty() && defaultLocaleFamilyName != englishLocaleFamilyName) {
                                QPlatformFontDatabase::registerFont(defaultLocaleFamilyName, defaultLocaleStyleName, QString(), weight, style, stretch, antialias, scalable, size, fixed, writingSystems, face);
                                face->AddRef();
                            }

                            face->Release();
                        }

                        names->Release();
                    }
                }

                font1->Release();
                font->Release();
            }
        }

        matchingFonts->Release();
    }
}

QFontEngine *QWindowsDirectWriteFontDatabase::fontEngine(const QFontDef &fontDef, void *handle)
{
    IDWriteFontFace *face = reinterpret_cast<IDWriteFontFace *>(handle);
    Q_ASSERT(face != nullptr);

    QWindowsFontEngineDirectWrite *fontEngine = new QWindowsFontEngineDirectWrite(face, fontDef.pixelSize, data());
    fontEngine->initFontInfo(fontDef, defaultVerticalDPI());

    return fontEngine;
}

QStringList QWindowsDirectWriteFontDatabase::fallbacksForFamily(const QString &family, QFont::Style style, QFont::StyleHint styleHint, QChar::Script script) const
{
    QStringList result;
    result.append(familyForStyleHint(styleHint));
    result.append(extraTryFontsForFamily(family));
    result.append(QPlatformFontDatabase::fallbacksForFamily(family, style, styleHint, script));

    qCDebug(lcQpaFonts) << __FUNCTION__ << family << style << styleHint
        << script << result;
    return result;
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

    IDWriteFontFace *face = createDirectWriteFace(loadedData);
    if (face == nullptr) {
        qCWarning(lcQpaFonts) << "Failed to create DirectWrite face from font data. Font may be unsupported.";
        return QStringList();
    }

    wchar_t defaultLocale[LOCALE_NAME_MAX_LENGTH];
    bool hasDefaultLocale = GetUserDefaultLocaleName(defaultLocale, LOCALE_NAME_MAX_LENGTH) != 0;
    wchar_t englishLocale[] = L"en-us";

    static const int SMOOTH_SCALABLE = 0xffff;
    const QString foundryName; // No such concept.
    const bool scalable = true;
    const bool antialias = false;
    const int size = SMOOTH_SCALABLE;

    QSupportedWritingSystems writingSystems;
    writingSystems.setSupported(QFontDatabase::Any);
    writingSystems.setSupported(QFontDatabase::Latin);

    QStringList ret;
    IDWriteFontFace3 *face3 = nullptr;
    if (SUCCEEDED(face->QueryInterface(__uuidof(IDWriteFontFace3),
                                      reinterpret_cast<void **>(&face3)))) {
        QString defaultLocaleFamilyName;
        QString englishLocaleFamilyName;

        IDWriteLocalizedStrings *names;
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

        QFont::Stretch stretch = fromDirectWriteStretch(face3->GetStretch());
        QFont::Style style = fromDirectWriteStyle(face3->GetStyle());
        QFont::Weight weight = fromDirectWriteWeight(face3->GetWeight());
        bool fixed = face3->IsMonospacedFont();

        qCDebug(lcQpaFonts) << "\tFont names:" << englishLocaleFamilyName << ", " << defaultLocaleFamilyName
                            << ", style names:" << englishLocaleStyleName << ", " << defaultLocaleStyleName
                            << ", stretch:" << stretch
                            << ", style:" << style
                            << ", weight:" << weight
                            << ", fixed:" << fixed;

        if (!englishLocaleFamilyName.isEmpty()) {
            if (applicationFont != nullptr) {
                QFontDatabasePrivate::ApplicationFont::Properties properties;
                properties.style = style;
                properties.weight = weight;
                properties.familyName = englishLocaleFamilyName;
                properties.styleName = englishLocaleStyleName;
                applicationFont->properties.append(properties);
            }

            ret.append(englishLocaleFamilyName);
            QPlatformFontDatabase::registerFont(englishLocaleFamilyName, englishLocaleStyleName, QString(), weight, style, stretch, antialias, scalable, size, fixed, writingSystems, face);
            face->AddRef();
        }

        if (!defaultLocaleFamilyName.isEmpty() && defaultLocaleFamilyName != englishLocaleFamilyName) {
            if (applicationFont != nullptr) {
                QFontDatabasePrivate::ApplicationFont::Properties properties;
                properties.style = style;
                properties.weight = weight;
                properties.familyName = englishLocaleFamilyName;
                properties.styleName = englishLocaleStyleName;
                applicationFont->properties.append(properties);
            }

            ret.append(defaultLocaleFamilyName);
            QPlatformFontDatabase::registerFont(defaultLocaleFamilyName, defaultLocaleStyleName, QString(), weight, style, stretch, antialias, scalable, size, fixed, writingSystems, face);
            face->AddRef();
        }

        face3->Release();
    } else {
        qCWarning(lcQpaFonts) << "Unable to query IDWriteFontFace3 interface from font face.";
    }

    face->Release();

    return ret;
}

void QWindowsDirectWriteFontDatabase::releaseHandle(void *handle)
{
    IDWriteFontFace *face = reinterpret_cast<IDWriteFontFace *>(handle);
    face->Release();
}

bool QWindowsDirectWriteFontDatabase::fontsAlwaysScalable() const
{
    return true;
}

bool QWindowsDirectWriteFontDatabase::isPrivateFontFamily(const QString &family) const
{
    Q_UNUSED(family);
    return false;
}

void QWindowsDirectWriteFontDatabase::populateFontDatabase()
{
    wchar_t defaultLocale[LOCALE_NAME_MAX_LENGTH];
    bool hasDefaultLocale = GetUserDefaultLocaleName(defaultLocale, LOCALE_NAME_MAX_LENGTH) != 0;
    wchar_t englishLocale[] = L"en-us";

    const QString defaultFontName = defaultFont().families().first();
    const QString systemDefaultFontName = systemDefaultFont().families().first();

    IDWriteFontCollection *fontCollection;
    if (SUCCEEDED(data()->directWriteFactory->GetSystemFontCollection(&fontCollection))) {
        for (uint i = 0; i < fontCollection->GetFontFamilyCount(); ++i) {
            IDWriteFontFamily *fontFamily;
            if (SUCCEEDED(fontCollection->GetFontFamily(i, &fontFamily))) {
                QString defaultLocaleName;
                QString englishLocaleName;

                IDWriteLocalizedStrings *names;
                if (SUCCEEDED(fontFamily->GetFamilyNames(&names))) {
                    if (hasDefaultLocale)
                        defaultLocaleName = localeString(names, defaultLocale);

                    englishLocaleName = localeString(names, englishLocale);
                }

                qCDebug(lcQpaFonts) << "Registering font, english name = " << englishLocaleName << ", name in current locale = " << defaultLocaleName;
                if (!defaultLocaleName.isEmpty()) {
                    registerFontFamily(defaultLocaleName);
                    m_populatedFonts.insert(defaultLocaleName, fontFamily);
                    fontFamily->AddRef();

                    if (defaultLocaleName == defaultFontName && defaultFontName != systemDefaultFontName) {
                        qDebug(lcQpaFonts) << "Adding default font" << systemDefaultFontName << "as alternative to" << defaultLocaleName;

                        m_populatedFonts.insert(systemDefaultFontName, fontFamily);
                        fontFamily->AddRef();
                    }
                }

                if (!englishLocaleName.isEmpty() && englishLocaleName != defaultLocaleName) {
                    registerFontFamily(englishLocaleName);
                    m_populatedFonts.insert(englishLocaleName, fontFamily);
                    fontFamily->AddRef();

                    if (englishLocaleName == defaultFontName && defaultFontName != systemDefaultFontName) {
                        qDebug(lcQpaFonts) << "Adding default font" << systemDefaultFontName << "as alternative to" << englishLocaleName;

                        m_populatedFonts.insert(systemDefaultFontName, fontFamily);
                        fontFamily->AddRef();
                    }
                }

                fontFamily->Release();
            }
        }
    }
}

QFont QWindowsDirectWriteFontDatabase::defaultFont() const
{
    return QFont(QStringLiteral("Segoe UI"));
}

QT_END_NAMESPACE
