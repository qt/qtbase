// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <QDir>

#include "qohosplatformfontdatabase_p.h"
#include <QtCore/private/qnapi_p.h>
#include <fontconfig/fontconfig.h>
#include <qohosplugincore.h>

using namespace std::string_literals;

QT_BEGIN_NAMESPACE

// Defined in gui/text/qfontdatabase.cpp
Q_GUI_EXPORT QFontDatabase::WritingSystem qt_writing_system_for_script(int script);

namespace {

enum class JsSystemFontType {
    ALL = 1 << 0,
    GENERIC = 1 << 1,
    STYLISH = 1 << 2,
    INSTALLED = 1 << 3,
};

bool ohosNoUiChildMode = false;

QStringList getSystemFontPaths(QtOhos::JsState &jsState)
{
    if (ohosNoUiChildMode)
        return {};

    auto fontModule = jsState.eval<QNapi::Object>("@ohos.font");

    auto systemFontPaths = QNapi::getArrayElements<QStringList, QNapi::String>(
        fontModule.call<QNapi::Array>("getSystemFontList"),
        [&](QNapi::String fontName) {
            auto fontInfo = fontModule.call("getFontByName", {fontName});
            return fontInfo.IsObject()
                ? QString::fromStdString(
                    QNapi::checkedCast<QNapi::Object>(fontInfo).get<QNapi::String>("path"))
                : QString();
        });

    systemFontPaths.removeAll(QString());

    return systemFontPaths;
}

QStringList getUIFontPaths(QtOhos::JsState &jsState)
{
    if (ohosNoUiChildMode)
        return {};

    auto uiFontDirs = QNapi::getArrayElements<QStringList, QNapi::String>(
        jsState.eval<QNapi::Array>("@ohos.font.getUIFontConfig().fontDir"),
        &QString::fromStdString);

    QStringList nameFilters;
    nameFilters << QLatin1String("*.ttf")
                << QLatin1String("*.otf")
                << QLatin1String("*.ttc");

    QStringList result;
    for (const auto &fontDir : uiFontDirs) {
        for (const QFileInfo &fileInfo : QDir(fontDir).entryInfoList(nameFilters, QDir::Files))
            result.append(fileInfo.absoluteFilePath());
    }

    return result;
}

QStringList getInstalledFontPaths()
{
    if (ohosNoUiChildMode)
        return {};

    auto fontsPaths = QtOhos::evalInJsThreadWithPromise<std::vector<std::string>>(
        [](QtOhos::JsState &jsState, QOhosTaskPromise<std::vector<std::string>> evalPromise) {
            jsState.eval<QNapi::Promise>(
                "@ohos.graphics.text.getSystemFontFullNamesByType(*)",
                {static_cast<int>(JsSystemFontType::INSTALLED)})
            .withContext(std::move(evalPromise))
            .onThenWithContext(
                [](const QtOhos::CallbackInfo &cbInfo, auto &evalPromise) {
                    auto fontsNamesArray = cbInfo.getFirstArg<QNapi::Array>(Q_FUNC_INFO);

                    if (fontsNamesArray.Length() == 0) {
                        evalPromise({});
                        return;
                    }

                    auto fontsNames = QNapi::getArrayElements<std::vector<std::string>, QNapi::String>(fontsNamesArray);
                    auto pathsCollector = std::make_shared<QOhosConsumer<std::string>>(
                        [evalPromise = std::move(evalPromise), pushSize = fontsNames.size(), result = std::vector<std::string>()](auto element) mutable {
                            result.push_back(std::move(element));
                            if (result.size() == pushSize)
                                evalPromise(std::move(result));
                        });

                    for (const auto &fontName : fontsNames) {
                        cbInfo.jsState().eval<QNapi::Promise>(
                            "@ohos.graphics.text.getFontDescriptorByFullName(*)",
                            {fontName, static_cast<int>(JsSystemFontType::INSTALLED)})
                        .onThen(
                            [pathsCollector](const QtOhos::CallbackInfo &cbInfo) {
                                auto fontDescriptor = cbInfo.getFirstArg<QNapi::Object>(Q_FUNC_INFO);
                                auto optFontPath = QNapi::getOptionalPropOrEmpty<QNapi::String>(fontDescriptor, "path");
                                (*pathsCollector)(!optFontPath.IsEmpty() ? optFontPath.Utf8Value() : ""s);
                            })
                        .onCatch(
                            [pathsCollector, fontName](const QtOhos::CallbackInfo &cbInfo) {
                                QtOhos::logJsCallbackError(
                                    cbInfo, ("getFontDescriptorByFullName("s + fontName + ") failed"s).c_str());
                                (*pathsCollector)(""s);
                            });
                    }
                })
            .onCatchWithContext(
                [](const QtOhos::CallbackInfo &cbInfo, auto &evalPromise) {
                    QtOhos::logJsCallbackError(cbInfo, "getSystemFontFullNamesByType() failed");
                    evalPromise({});
                });
        });

    QStringList result;
    std::transform(
        fontsPaths.begin(), fontsPaths.end(),
        std::back_inserter(result), QString::fromStdString);
    result.removeAll(QString());
    result.sort();

    return result;
}

std::string getDefaultFontFamily(QtOhos::JsState &jsState)
{
    if (ohosNoUiChildMode)
        return "";

    std::string defaultFontFamily;
    auto genericFonts = jsState.eval<QNapi::Array>("@ohos.font.getUIFontConfig().generic");
    if (genericFonts.Length() > 0) {
        auto firstFont = QNapi::checkedCast<QNapi::Object>(genericFonts.Get(0U));
        defaultFontFamily = firstFont.get<QNapi::String>("family");
    } else {
        qFatal("Failed to get system default font family name."
            " The reason is: empty `@ohos.font.getUIFontConfig().generic` array.");
    }

    return defaultFontFamily;
}

void registerSystemFonts()
{
    QStringList fontPaths;
    QtOhos::runInJsThreadAndWait(
        [&](auto &jsState) {
            fontPaths.append(getSystemFontPaths(jsState));
            fontPaths.append(getUIFontPaths(jsState));
        });
    fontPaths.append(getInstalledFontPaths());
    fontPaths.removeDuplicates();

    QSet<QString> uniqueFontDirs;
    for (const QString &fontPath : fontPaths)
        uniqueFontDirs.insert(QFileInfo(fontPath).absolutePath());

    FcConfig *config = FcConfigGetCurrent();
    if (config != nullptr) {
        for (const QString &dir : uniqueFontDirs)
            FcConfigAppFontAddDir(config, reinterpret_cast<const FcChar8 *>(dir.toUtf8().constData()));
        FcConfigBuildFonts(config);
    } else {
        qOhosPrintfError("Failed to get fontconfig current configuration.");
        std::abort();
    }
}

}

void QOhosPlatformFontDatabase::setOhosNoUiChildMode()
{
    ohosNoUiChildMode = true;
}

void QOhosPlatformFontDatabase::populateFontDatabase()
{
    FcInit();

    registerSystemFonts();

    QFontconfigDatabase::populateFontDatabase();
}

QStringList QOhosPlatformFontDatabase::fallbacksForFamily(const QString &family,
                                                             QFont::Style style,
                                                             QFont::StyleHint styleHint,
                                                             QFontDatabasePrivate::ExtendedScript script) const
{
    QStringList result;

    const QFontDatabase::WritingSystem ws = qt_writing_system_for_script(script);
    const bool defaultFontSupportsScript =
        ws == QFontDatabase::Any ||
        QFontDatabase::writingSystems(defaultFont().family()).contains(ws);

    if (defaultFontSupportsScript)
        result.append(defaultFont().family());

    if (styleHint == QFont::Monospace || styleHint == QFont::Courier)
        result.append(QString::fromUtf8(qgetenv("Droid Sans Mono;Droid Sans;Noto Sans")).split(QLatin1Char(';')));
    else if (styleHint == QFont::Serif)
        result.append(QString::fromUtf8(qgetenv("Noto Serif")).split(QLatin1Char(';')));
    else
        result.append(QString::fromUtf8(qgetenv("Roboto;Droid Sans")).split(QLatin1Char(';')));
    result.append(QFontconfigDatabase::fallbacksForFamily(family, style, styleHint, script));

    return result;
}

QFont QOhosPlatformFontDatabase::defaultFont() const
{
    static const char * const preferredDefaultFontFamily = "HarmonyOS Sans SC";
    static const QString defaultFontFamily =
        QFontDatabase::families().contains(QLatin1String(preferredDefaultFontFamily))
            ? QString::fromStdString(preferredDefaultFontFamily)
            : QtOhos::evalInJsThread(
                [&](auto &jsState) {
                    return QString::fromStdString(getDefaultFontFamily(jsState));
                });
    return QFont(defaultFontFamily);
}

QT_END_NAMESPACE
