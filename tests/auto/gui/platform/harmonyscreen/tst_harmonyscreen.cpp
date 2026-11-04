// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtTest/qtest.h>

#include <QtGui/qguiapplication.h>
#include <QtGui/qscreen.h>
#include <QtGui/qwindow.h>

#include <QtCore/qcoreapplication.h>
#include <QtCore/qharmonyjscontext.h>
#include <QtCore/qharmonyjsobjectref.h>

#include <cstdint>
#include <optional>
#include <vector>

#include <napi/native_api.h>

#include "../../../../shared/harmonytestutils.h"

using namespace QTestHarmony;

using QHarmonyApplication = QNativeInterface::QHarmonyApplication;
using QHarmonyScreen = QNativeInterface::QHarmonyScreen;
using QHarmonyWindow = QNativeInterface::QHarmonyWindow;

namespace {

napi_value callJsMethod(napi_env env, napi_value objectValue, const char *name, std::vector<napi_value> args = {})
{
    const auto functionValue = getJsProperty(env, objectValue, name);
    if (jsTypeOf(env, functionValue) != napi_function)
        return nullptr;

    napi_value resultValue = nullptr;
    checkNapiStatus(
        napi_call_function(env, objectValue, functionValue, args.size(), args.data(), &resultValue),
        "napi_call_function");
    return resultValue;
}

std::optional<std::int64_t> getJsInt64Property(napi_env env, napi_value objectValue, const char *name)
{
    const auto propertyValue = getJsProperty(env, objectValue, name);
    if (jsTypeOf(env, propertyValue) != napi_number)
        return std::nullopt;

    std::int64_t value = 0;
    checkNapiStatus(napi_get_value_int64(env, propertyValue, &value), "napi_get_value_int64");
    return value;
}

std::optional<std::int64_t> fetchPlatformDisplayId(std::int64_t displayId)
{
    return QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            napi_value displayIdValue = nullptr;
            checkNapiStatus(napi_create_int64(env, displayId, &displayIdValue), "napi_create_int64");
            const auto displayValue = callJsMethod(
                env, jsContext.tryImportModule("@ohos.display"), "getDisplayByIdSync", {displayIdValue});
            return getJsInt64Property(env, displayValue, "id");
        });
}

std::optional<std::int64_t> fetchJsWindowDisplayId(const QHarmonyJsObjectRef &jsWindowRef)
{
    return QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            return getJsInt64Property(env, callJsMethod(env, jsWindowRef.value(jsContext), "getWindowProperties"), "displayId");
        });
}

}

class tst_HarmonyScreen : public QObject
{
    Q_OBJECT

private slots:
    void cleanup();

    void screen_displayId_identifiesPlatformDisplay();
    void screen_displayId_matchesDisplayOfShownWindow();
};

void tst_HarmonyScreen::cleanup()
{
    verifyNoNapiCallFailures();
}

void tst_HarmonyScreen::screen_displayId_identifiesPlatformDisplay()
{
    const auto screens = QGuiApplication::screens();

    QVERIFY(!screens.isEmpty());
    for (auto *screen : screens) {
        auto *harmonyScreen = screen->nativeInterface<QHarmonyScreen>();
        QVERIFY(harmonyScreen);

        const auto displayId = harmonyScreen->displayId();
        const auto optPlatformDisplayId = fetchPlatformDisplayId(displayId);

        QVERIFY(optPlatformDisplayId);
        QCOMPARE(*optPlatformDisplayId, static_cast<std::int64_t>(displayId));
    }
}

void tst_HarmonyScreen::screen_displayId_matchesDisplayOfShownWindow()
{
    QWindow window;
    window.show();
    QVERIFY(QTest::qWaitForWindowExposed(&window));
    auto *harmonyWindow = window.nativeInterface<QHarmonyWindow>();
    QVERIFY(harmonyWindow);
    auto *harmonyScreen = window.screen()->nativeInterface<QHarmonyScreen>();
    QVERIFY(harmonyScreen);

    const auto jsWindowRef = harmonyWindow->jsWindow();
    QVERIFY(!jsWindowRef.isNull());

    const auto optJsWindowDisplayId = fetchJsWindowDisplayId(jsWindowRef);
    if (!optJsWindowDisplayId)
        QSKIP("The window properties do not report a display ID");

    QCOMPARE(*optJsWindowDisplayId, static_cast<std::int64_t>(harmonyScreen->displayId()));
}

QTEST_MAIN(tst_HarmonyScreen)
#include "tst_harmonyscreen.moc"
