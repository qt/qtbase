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

QT_BEGIN_NAMESPACE

using namespace emscripten;
using namespace Qt::StringLiterals;

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

    // check if local-fonts API is available in the browser
    val window = val::global("window");
    val fonts = window["queryLocalFonts"];

    if (fonts.isUndefined())
        return;

    val permissions = val::global("navigator")["permissions"];
    if (permissions["request"].isUndefined())
        return;

    val requestLocalFontsPermission = val::object();
    requestLocalFontsPermission.set("name", std::string("local-fonts"));

    qstdweb::PromiseCallbacks permissionRequestCallbacks {
        .thenFunc = [window](val status) {
            qCDebug(lcQpaFonts) << "onFontPermissionSuccess:"
                << QString::fromStdString(status["state"].as<std::string>());

            // query all available local fonts and call registerFontFamily for each of them
            qstdweb::Promise::make(window, "queryLocalFonts", {
                .thenFunc = [](val status) {
                    const int count = status["length"].as<int>();
                    for (int i = 0; i < count; ++i) {
                        val font = status.call<val>("at", i);
                        const std::string family = font["family"].as<std::string>();
                        QFreeTypeFontDatabase::registerFontFamily(QString::fromStdString(family));
                    }
                    QWasmFontDatabase::notifyFontsChanged();
                },
                .catchFunc = [](val) {
                    qCWarning(lcQpaFonts)
                        << "Error while trying to query local-fonts API";
                }
            });
        },
        .catchFunc = [](val error) {
            qCWarning(lcQpaFonts)
                << "Error while requesting local-fonts API permission: "
                << QString::fromStdString(error["name"].as<std::string>());
        }
    };

    // request local fonts permission (currently supported only by Chrome 103+)
    qstdweb::Promise::make(permissions, "request", std::move(permissionRequestCallbacks), std::move(requestLocalFontsPermission));
}

void QWasmFontDatabase::populateFamily(const QString &familyName)
{
    val window = val::global("window");

    auto queryFontsArgument = val::array(std::vector<val>({ val(familyName.toStdString()) }));
    val queryFont = val::object();
    queryFont.set("postscriptNames", std::move(queryFontsArgument));

    qstdweb::PromiseCallbacks localFontsQueryCallback {
        .thenFunc = [](val status) {
            val font = status.call<val>("at", 0);

            if (font.isUndefined())
                return;

            qstdweb::PromiseCallbacks blobQueryCallback {
                .thenFunc = [](val status) {
                    qCDebug(lcQpaFonts) << "onBlobQuerySuccess";

                    qstdweb::PromiseCallbacks arrayBufferCallback {
                        .thenFunc = [](val status) {
                            qCDebug(lcQpaFonts) << "onArrayBuffer" ;

                            QByteArray fontByteArray = QByteArray::fromEcmaUint8Array(status);

                            QFreeTypeFontDatabase::addTTFile(fontByteArray, QByteArray());

                            QWasmFontDatabase::notifyFontsChanged();
                        },
                        .catchFunc = [](val) {
                            qCWarning(lcQpaFonts) << "onArrayBufferError";
                        }
                    };

                    qstdweb::Promise::make(status, "arrayBuffer", std::move(arrayBufferCallback));
                },
                .catchFunc = [](val) {
                    qCWarning(lcQpaFonts) << "onBlobQueryError";
                }
            };

            qstdweb::Promise::make(font, "blob", std::move(blobQueryCallback));
        },
        .catchFunc = [](val) {
            qCWarning(lcQpaFonts) << "onLocalFontsQueryError";
        }
    };

    qstdweb::Promise::make(window, "queryLocalFonts", std::move(localFontsQueryCallback), std::move(queryFont));
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
