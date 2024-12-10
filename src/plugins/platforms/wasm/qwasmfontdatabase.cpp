// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmfontdatabase.h"
#include "qwasmintegration.h"

#include <QtCore/qfile.h>
#include <QtCore/private/qstdweb_p.h>
#include <QtCore/private/qeventdispatcher_wasm_p.h>
#include <QtGui/private/qguiapplication_p.h>

#include <emscripten.h>
#include <emscripten/val.h>
#include <emscripten/bind.h>

#include <map>
#include <array>

QT_BEGIN_NAMESPACE

using namespace emscripten;
using namespace Qt::StringLiterals;


namespace {

class FontData
{
public:
    FontData(val fontData)
        :m_fontData(fontData) {}

    QString family() const
    {
        return QString::fromStdString(m_fontData["family"].as<std::string>());
    }

    QString fullName() const
    {
        return QString::fromStdString(m_fontData["fullName"].as<std::string>());
    }

    QString postscriptName() const
    {
        return QString::fromStdString(m_fontData["postscriptName"].as<std::string>());
    }

    QString style() const
    {
        return QString::fromStdString(m_fontData["style"].as<std::string>());
    }

    val value() const
    {
        return m_fontData;
    }

private:
    val m_fontData;
};

val makeObject(const char *key, const char *value)
{
    val obj = val::object();
    obj.set(key, std::string(value));
    return obj;
}

void printError(val err) {
    qCWarning(lcQpaFonts)
        << QString::fromStdString(err["name"].as<std::string>())
        << QString::fromStdString(err["message"].as<std::string>());
    QWasmFontDatabase::endAllFontFileLoading();
}

void checkFontAccessPermitted(std::function<void(bool)> callback)
{
    const val permissions = val::global("navigator")["permissions"];
    if (permissions.isUndefined()) {
        callback(false);
        return;
    }

    qstdweb::Promise::make(permissions, "query", {
        .thenFunc = [callback](val status) {
            callback(status["state"].as<std::string>() == "granted");
        },
    }, makeObject("name", "local-fonts"));
}

void queryLocalFonts(std::function<void(const QList<FontData> &)> callback)
{
    emscripten::val window = emscripten::val::global("window");
    qstdweb::Promise::make(window, "queryLocalFonts", {
        .thenFunc = [callback](emscripten::val fontArray) {
            QList<FontData> fonts;
            const int count = fontArray["length"].as<int>();
            fonts.reserve(count);
            for (int i = 0; i < count; ++i)
                fonts.append(FontData(fontArray.call<emscripten::val>("at", i)));
            callback(fonts);
        },
        .catchFunc = printError
    });
}

void readBlob(val blob, std::function<void(const QByteArray &)> callback)
{
    qstdweb::Promise::make(blob, "arrayBuffer", {
        .thenFunc = [callback](emscripten::val fontArrayBuffer) {
            QByteArray fontData = qstdweb::Uint8Array(qstdweb::ArrayBuffer(fontArrayBuffer)).copyToQByteArray();
            callback(fontData);
        },
        .catchFunc = printError
    });
}

void readFont(FontData font, std::function<void(const QByteArray &)> callback)
{
    qstdweb::Promise::make(font.value(), "blob", {
        .thenFunc = [callback](val blob) {
            readBlob(blob, [callback](const QByteArray &data) {
                callback(data);
            });
        },
        .catchFunc = printError
    });
}

emscripten::val getLocalFontsConfigProperty(const char *name) {
    emscripten::val qt = val::module_property("qt");
    if (qt.isUndefined())
        return emscripten::val();
    emscripten::val localFonts = qt["localFonts"];
    if (localFonts.isUndefined())
        return emscripten::val();
    return localFonts[name];
};

bool getLocalFontsBoolConfigPropertyWithDefault(const char *name, bool defaultValue) {
    emscripten::val prop = getLocalFontsConfigProperty(name);
    if (prop.isUndefined())
        return defaultValue;
    return prop.as<bool>();
};

QString getLocalFontsStringConfigPropertyWithDefault(const char *name, QString defaultValue) {
    emscripten::val prop = getLocalFontsConfigProperty(name);
    if (prop.isUndefined())
        return defaultValue;
    return QString::fromStdString(prop.as<std::string>());
};

QStringList getLocalFontsStringListConfigPropertyWithDefault(const char *name, QStringList defaultValue) {
    emscripten::val array = getLocalFontsConfigProperty(name);
    if (array.isUndefined())
        return defaultValue;

    QStringList list;
    int size = array["length"].as<int>();
    for (int i = 0; i < size; ++i) {
        emscripten::val element = array.call<emscripten::val>("at", i);
        QString string = QString::fromStdString(element.as<std::string>());
        if (!string.isEmpty())
            list.append(string);
    }
    return list;
};

} // namespace

QWasmFontDatabase::QWasmFontDatabase()
:QFreeTypeFontDatabase()
{
    m_localFontsApiSupported = val::global("window")["queryLocalFonts"].isUndefined() == false;
    if (m_localFontsApiSupported)
        beginFontDatabaseStartupTask();
}

QWasmFontDatabase *QWasmFontDatabase::get()
{
    return static_cast<QWasmFontDatabase *>(QWasmIntegration::get()->fontDatabase());
}

// Populates the font database with local fonts. Will make the browser ask
// the user for permission if needed. Does nothing if the Local Font Access API
// is not supported.
void QWasmFontDatabase::populateLocalfonts()
{
    // Decide which font families to populate based on user preferences
    QStringList selectedLocalFontFamilies;
    bool allFamilies = false;

    switch (m_localFontFamilyLoadSet) {
    case NoFontFamilies:
    default:
        // keep empty selectedLocalFontFamilies
    break;
    case DefaultFontFamilies: {
        const QStringList webSafeFontFamilies =
            {"Arial", "Verdana", "Tahoma", "Trebuchet", "Times New Roman",
             "Georgia", "Garamond", "Courier New"};
        selectedLocalFontFamilies = webSafeFontFamilies;
    } break;
    case AllFontFamilies:
        allFamilies = true;
    break;
    }

    selectedLocalFontFamilies += m_extraLocalFontFamilies;

    if (selectedLocalFontFamilies.isEmpty() && !allFamilies) {
        endAllFontFileLoading();
        return;
    }

    populateLocalFontFamilies(selectedLocalFontFamilies, allFamilies);
}

namespace {
    QStringList toStringList(emscripten::val array)
    {
        QStringList list;
        int size = array["length"].as<int>();
        for (int i = 0; i < size; ++i) {
            emscripten::val element = array.call<emscripten::val>("at", i);
            QString string = QString::fromStdString(element.as<std::string>());
            if (!string.isEmpty())
                list.append(string);
        }
        return list;
    }
}

void QWasmFontDatabase::populateLocalFontFamilies(emscripten::val families)
{
    if (!m_localFontsApiSupported)
        return;
    populateLocalFontFamilies(toStringList(families), false);
}

void QWasmFontDatabase::populateLocalFontFamilies(const QStringList &fontFamilies, bool allFamilies)
{
    queryLocalFonts([fontFamilies, allFamilies](const QList<FontData> &fonts) {
        refFontFileLoading();
        QList<FontData> filteredFonts;
        std::copy_if(fonts.begin(), fonts.end(), std::back_inserter(filteredFonts),
            [fontFamilies, allFamilies](FontData fontData) {
                return allFamilies || fontFamilies.contains(fontData.family());
        });

        for (const FontData &font: filteredFonts) {
            refFontFileLoading();
            readFont(font, [font](const QByteArray &fontData){
                QFreeTypeFontDatabase::registerFontFamily(font.family());
                QFreeTypeFontDatabase::addTTFile(fontData, QByteArray());
                derefFontFileLoading();
            });
        }
        derefFontFileLoading();
    });

}

void QWasmFontDatabase::populateFontDatabase()
{
    // Load bundled font file from resources.
    const QString fontFileNames[] = {
        QStringLiteral(":/fonts/DejaVuSansMono.ttf"),
        QStringLiteral(":/fonts/DejaVuSans.ttf"),
    };
    for (const QString &fontFileName : fontFileNames) {
        QFile theFont(fontFileName);
        if (!theFont.open(QIODevice::ReadOnly))
            break;

        QFreeTypeFontDatabase::addTTFile(theFont.readAll(), fontFileName.toLatin1());
    }

    // Get config options for controlling local fonts usage
    m_queryLocalFontsPermission = getLocalFontsBoolConfigPropertyWithDefault("requestPermission", false);
    QString fontFamilyLoadSet = getLocalFontsStringConfigPropertyWithDefault("familiesCollection", "DefaultFontFamilies");
    m_extraLocalFontFamilies = getLocalFontsStringListConfigPropertyWithDefault("extraFamilies", QStringList());

    if (fontFamilyLoadSet == "NoFontFamilies") {
        m_localFontFamilyLoadSet = NoFontFamilies;
    } else if (fontFamilyLoadSet == "DefaultFontFamilies") {
        m_localFontFamilyLoadSet = DefaultFontFamilies;
    } else if (fontFamilyLoadSet == "AllFontFamilies") {
        m_localFontFamilyLoadSet = AllFontFamilies;
    } else {
        m_localFontFamilyLoadSet = NoFontFamilies;
        qWarning() << "Unknown fontFamilyLoadSet value" << fontFamilyLoadSet;
    }

    if (!m_localFontsApiSupported)
        return;

    // Populate the font database with local fonts. Either try unconditianlly
    // if displyaing a fonts permissions dialog at startup is allowed, or else
    // only if we already have permission.
    if (m_queryLocalFontsPermission) {
        populateLocalfonts();
    } else {
        checkFontAccessPermitted([this](bool granted) {
            if (granted)
                populateLocalfonts();
            else
                endAllFontFileLoading();
        });
    }
}

QFontEngine *QWasmFontDatabase::fontEngine(const QFontDef &fontDef, void *handle)
{
    QFontEngine *fontEngine = QFreeTypeFontDatabase::fontEngine(fontDef, handle);
    return fontEngine;
}

QStringList QWasmFontDatabase::fallbacksForFamily(const QString &family, QFont::Style style,
                                                    QFont::StyleHint styleHint,
                                                    QFontDatabasePrivate::ExtendedScript script) const
{
    QStringList fallbacks
        = QFreeTypeFontDatabase::fallbacksForFamily(family, style, styleHint, script);

    // Add the DejaVuSans.ttf font (loaded in populateFontDatabase above) as a falback font
    // to all other fonts (except itself).
    static const QString wasmFallbackFonts[] = { "DejaVu Sans" };
    for (auto wasmFallbackFont : wasmFallbackFonts) {
        if (family != wasmFallbackFont && !fallbacks.contains(wasmFallbackFont))
            fallbacks.append(wasmFallbackFont);
    }

    return fallbacks;
}

void QWasmFontDatabase::releaseHandle(void *handle)
{
    QFreeTypeFontDatabase::releaseHandle(handle);
}

QFont QWasmFontDatabase::defaultFont() const
{
    return QFont("DejaVu Sans"_L1);
}

namespace {
    int g_pendingFonts = 0;
    bool g_fontStartupTaskCompleted = false;
}

// Registers font loading as a startup task, which makes Qt delay
// sending onLoaded event until font loading has completed.
void QWasmFontDatabase::beginFontDatabaseStartupTask()
{
    g_fontStartupTaskCompleted = false;
    QEventDispatcherWasm::registerStartupTask();
}

// Ends the font loading startup task.
void QWasmFontDatabase::endFontDatabaseStartupTask()
{
    if (!g_fontStartupTaskCompleted) {
        g_fontStartupTaskCompleted = true;
        QEventDispatcherWasm::completeStarupTask();
    }
}

// Registers that a font file will be loaded.
void QWasmFontDatabase::refFontFileLoading()
{
    g_pendingFonts += 1;
}

// Registers that one font file has been loaded, and sends notifactions
// when all pending font files have been loaded.
void QWasmFontDatabase::derefFontFileLoading()
{
    if (--g_pendingFonts <= 0) {
        QFontCache::instance()->clear();
        emit qGuiApp->fontDatabaseChanged();
        endFontDatabaseStartupTask();
    }
}

// Unconditionally ends local font loading, for instance if there
// are no fonts to load or if there was an unexpected error.
void QWasmFontDatabase::endAllFontFileLoading()
{
    bool hadPandingfonts = g_pendingFonts > 0;
    if (hadPandingfonts) {
        // The hadPandingfonts counter might no longer be correct; disable counting
        // and send notifications unconditionally.
        g_pendingFonts = 0;
        QFontCache::instance()->clear();
        emit qGuiApp->fontDatabaseChanged();
    }

    endFontDatabaseStartupTask();
}


QT_END_NAMESPACE
