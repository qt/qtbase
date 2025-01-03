// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include "qwasmfontdatabase.h"
#include "qwasmintegration.h"

#include <QtCore/qfile.h>
#include <QtCore/private/qstdweb_p.h>
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

bool isLocalFontsAPISupported()
{
    return val::global("window")["queryLocalFonts"].isUndefined() == false;
}

val makeObject(const char *key, const char *value)
{
    val obj = val::object();
    obj.set(key, std::string(value));
    return obj;
}

std::multimap<QString, emscripten::val> makeFontFamilyMap(const QList<val> &fonts)
{
    std::multimap<QString, emscripten::val> fontFamilies;
    for (auto font : fonts) {
        QString family = QString::fromStdString(font["family"].as<std::string>());
        fontFamilies.insert(std::make_pair(family, font));
    }
    return fontFamilies;
}

void printError(val err) {
    qCWarning(lcQpaFonts)
        << QString::fromStdString(err["name"].as<std::string>())
        << QString::fromStdString(err["message"].as<std::string>());
}

std::array<const char *, 8> webSafeFontFamilies()
{
    return {"Arial", "Verdana", "Tahoma", "Trebuchet", "Times New Roman",
            "Georgia", "Garamond", "Courier New"};
}

void checkFontAccessPermitted(std::function<void()> callback)
{
    const val permissions = val::global("navigator")["permissions"];
    if (permissions.isUndefined())
        return;

    qstdweb::Promise::make(permissions, "query", {
        .thenFunc = [callback](val status) {
            if (status["state"].as<std::string>() == "granted")
                callback();
        }
    }, makeObject("name", "local-fonts"));
}

void queryLocalFonts(std::function<void(const QList<val> &)> callback)
{
    emscripten::val window = emscripten::val::global("window");
    qstdweb::Promise::make(window, "queryLocalFonts", {
        .thenFunc = [callback](emscripten::val fontArray) {
            QList<val> fonts;
            const int count = fontArray["length"].as<int>();
            fonts.reserve(count);
            for (int i = 0; i < count; ++i)
                fonts.append(fontArray.call<emscripten::val>("at", i));
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

void readFont(val font, std::function<void(const QByteArray &)> callback)
{
    qstdweb::Promise::make(font, "blob", {
        .thenFunc = [callback](val blob) {
            readBlob(blob, [callback](const QByteArray &data) {
                callback(data);
            });
        },
        .catchFunc = printError
    });
}

} // namespace

void QWasmFontDatabase::populateLocalfonts()
{
    if (!isLocalFontsAPISupported())
        return;

    // Run the font population code if local font access has been
    // permitted. This does not request permission, since we are currently
    // starting up and should not display a permission request dialog at
    // this point.
    checkFontAccessPermitted([](){
        queryLocalFonts([](const QList<val> &fonts){
            auto fontFamilies = makeFontFamilyMap(fonts);
            // Populate some font families. We can't populate _all_ fonts as in-memory fonts,
            // since that would require several gigabytes of memory. Instead, populate
            // a subset of the available fonts.
            for (const QString &family: webSafeFontFamilies()) {
                auto fontsRange = fontFamilies.equal_range(family);
                if (fontsRange.first != fontsRange.second)
                    QFreeTypeFontDatabase::registerFontFamily(family);

                for (auto it = fontsRange.first; it != fontsRange.second; ++it) {
                    const val font = it->second;
                    readFont(font, [](const QByteArray &fontData){
                        QFreeTypeFontDatabase::addTTFile(fontData, QByteArray());
                        QWasmFontDatabase::notifyFontsChanged();
                    });
                }
            }
        });
    });
}

void QWasmFontDatabase::populateFontDatabase()
{
    // Load font file from resources. Currently
    // all fonts needs to be bundled with the nexe
    // as Qt resources.

    const QString fontFileNames[] = {
        QStringLiteral(":/fonts/DejaVuSansMono.ttf"),
        QStringLiteral(":/fonts/Vera.ttf"),
        QStringLiteral(":/fonts/DejaVuSans.ttf"),
    };
    for (const QString &fontFileName : fontFileNames) {
        QFile theFont(fontFileName);
        if (!theFont.open(QIODevice::ReadOnly))
            break;

        QFreeTypeFontDatabase::addTTFile(theFont.readAll(), fontFileName.toLatin1());
    }

    populateLocalfonts();
}

QFontEngine *QWasmFontDatabase::fontEngine(const QFontDef &fontDef, void *handle)
{
    return QFreeTypeFontDatabase::fontEngine(fontDef, handle);
}

QStringList QWasmFontDatabase::fallbacksForFamily(const QString &family, QFont::Style style,
                                                    QFont::StyleHint styleHint,
                                                    QChar::Script script) const
{
    QStringList fallbacks
        = QFreeTypeFontDatabase::fallbacksForFamily(family, style, styleHint, script);

    // Add the vera.ttf and DejaVuSans.ttf fonts (loaded in populateFontDatabase above) as falback fonts
    // to all other fonts (except itself).
    static const QString wasmFallbackFonts[] = { "Bitstream Vera Sans", "DejaVu Sans" };
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
    return QFont("Bitstream Vera Sans"_L1);
}

void QWasmFontDatabase::notifyFontsChanged()
{
    QFontCache::instance()->clear();
    emit qGuiApp->fontDatabaseChanged();
}

QT_END_NAMESPACE
