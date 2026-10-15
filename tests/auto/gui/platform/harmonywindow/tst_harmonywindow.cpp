// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <QtCore/qcoreapplication.h>
#include <QtCore/qharmonyappjscontext.h>
#include <QtCore/qharmonyjscontext.h>
#include <QtCore/qharmonyjsobjectref.h>
#include <QtGui/qwindow.h>
#include <QtTest/qtest.h>
#include <mutex>
#include <napi/native_api.h>
#include <optional>
#include <string>
#include <vector>

using QHarmonyApplication = QNativeInterface::QHarmonyApplication;
using QHarmonyWindow = QNativeInterface::QHarmonyWindow;

namespace {

std::mutex napiCallFailuresMutex;
std::vector<std::string> napiCallFailures;

void checkNapiStatus(napi_status status, const char *callName)
{
    if (status != napi_ok) {
        std::lock_guard<std::mutex> napiCallFailuresLock(napiCallFailuresMutex);
        napiCallFailures.push_back(std::string(callName) + ": " + std::to_string(static_cast<int>(status)));
    }
}

std::vector<std::string> takeNapiCallFailures()
{
    std::lock_guard<std::mutex> napiCallFailuresLock(napiCallFailuresMutex);
    return std::exchange(napiCallFailures, {});
}

napi_valuetype jsTypeOf(napi_env env, napi_value value)
{
    if (!value)
        return napi_undefined;

    napi_valuetype type = napi_undefined;
    checkNapiStatus(napi_typeof(env, value, &type), "napi_typeof");
    return type;
}

napi_value getJsProperty(napi_env env, napi_value objectValue, const char *name)
{
    if (!objectValue)
        return nullptr;

    napi_value propertyValue = nullptr;
    checkNapiStatus(napi_get_named_property(env, objectValue, name, &propertyValue), "napi_get_named_property");
    return propertyValue;
}

napi_value callJsMethod(napi_env env, napi_value objectValue, const char *name)
{
    const auto functionValue = getJsProperty(env, objectValue, name);
    if (jsTypeOf(env, functionValue) != napi_function)
        return nullptr;

    napi_value resultValue = nullptr;
    checkNapiStatus(napi_call_function(env, objectValue, functionValue, 0, nullptr, &resultValue), "napi_call_function");
    return resultValue;
}

std::optional<double> getJsWindowId(napi_env env, napi_value windowValue)
{
    const auto idValue = getJsProperty(env, callJsMethod(env, windowValue, "getWindowProperties"), "id");
    if (jsTypeOf(env, idValue) != napi_number)
        return std::nullopt;

    double id = 0;
    checkNapiStatus(napi_get_value_double(env, idValue, &id), "napi_get_value_double");
    return id;
}

std::optional<double> fetchJsWindowId(const QHarmonyJsObjectRef &jsWindowRef)
{
    return QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            return getJsWindowId(jsContext.env(), jsWindowRef.value(jsContext));
        });
}

std::optional<double> fetchAbilityMainWindowId(const QHarmonyJsObjectRef &abilityRef)
{
    return QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto contextValue = getJsProperty(env, abilityRef.value(jsContext), "context");
            const auto windowStageValue = getJsProperty(env, contextValue, "windowStage");
            return getJsWindowId(env, callJsMethod(env, windowStageValue, "getMainWindowSync"));
        });
}

struct MainWindowIds
{
    std::optional<double> optJsWindowId;
    std::optional<double> optAbilityMainWindowId;
};

MainWindowIds fetchMainWindowIds(QHarmonyWindow *harmonyWindow)
{
    return MainWindowIds{
        .optJsWindowId = fetchJsWindowId(harmonyWindow->jsWindow()),
        .optAbilityMainWindowId = fetchAbilityMainWindowId(harmonyWindow->ability()),
    };
}

QHarmonyJsObjectRef fetchDefaultAbility()
{
    return QHarmonyApplication::runOnJsThreadAndWait(
        [](QHarmonyJsContext &jsContext) {
            return QHarmonyJsObjectRef::fromValue(jsContext, QHarmonyAppJsContext::fromJsContext(jsContext).defaultAbility());
        });
}

}

class tst_HarmonyWindow : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanup();

    void window_ability_firstWindowIsDefaultAbility();
    void window_jsWindow_isMainWindowOfAbility();
    void window_abilityAndJsWindow_stable();
    void window_abilityAndJsWindow_childWindow();
    void window_abilityAndJsWindow_secondTopLevelWindow();
    void window_abilityAndJsWindow_transientWindow();

    void appJsContext_defaultAbility_unchangedByNewWindow();

private:
    QWindow m_firstWindow;
    QWindow m_secondWindow;
    QHarmonyWindow *m_firstHarmonyWindow = nullptr;
    QHarmonyWindow *m_secondHarmonyWindow = nullptr;
};

void tst_HarmonyWindow::initTestCase()
{
    m_firstWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&m_firstWindow));
    m_firstHarmonyWindow = m_firstWindow.nativeInterface<QHarmonyWindow>();
    QVERIFY(m_firstHarmonyWindow);

    m_secondWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&m_secondWindow));
    m_secondHarmonyWindow = m_secondWindow.nativeInterface<QHarmonyWindow>();
    QVERIFY(m_secondHarmonyWindow);
}

void tst_HarmonyWindow::cleanup()
{
    const auto failures = takeNapiCallFailures();
    std::string failuresText;
    for (const auto &failure : failures) {
        if (!failuresText.empty())
            failuresText += ", ";
        failuresText += failure;
    }
    const auto message = "Node-API calls failed: " + failuresText;
    QVERIFY2(failures.empty(), message.c_str());
}

void tst_HarmonyWindow::window_ability_firstWindowIsDefaultAbility()
{
    const auto defaultAbilityRef = fetchDefaultAbility();

    QVERIFY(!defaultAbilityRef.isNull());
    QCOMPARE(m_firstHarmonyWindow->ability(), defaultAbilityRef);
}

void tst_HarmonyWindow::window_jsWindow_isMainWindowOfAbility()
{
    const auto firstIds = fetchMainWindowIds(m_firstHarmonyWindow);
    const auto secondIds = fetchMainWindowIds(m_secondHarmonyWindow);

    QVERIFY(firstIds.optJsWindowId);
    QVERIFY(firstIds.optAbilityMainWindowId);
    QCOMPARE(*firstIds.optJsWindowId, *firstIds.optAbilityMainWindowId);
    QVERIFY(secondIds.optJsWindowId);
    QVERIFY(secondIds.optAbilityMainWindowId);
    QCOMPARE(*secondIds.optJsWindowId, *secondIds.optAbilityMainWindowId);
}

void tst_HarmonyWindow::window_abilityAndJsWindow_stable()
{
    const auto abilityRef = m_firstHarmonyWindow->ability();
    const auto jsWindowRef = m_firstHarmonyWindow->jsWindow();

    QVERIFY(!abilityRef.isNull());
    QVERIFY(!jsWindowRef.isNull());
    QCOMPARE(m_firstHarmonyWindow->ability(), abilityRef);
    QCOMPARE(m_firstHarmonyWindow->jsWindow(), jsWindowRef);
}

void tst_HarmonyWindow::window_abilityAndJsWindow_childWindow()
{
    QWindow childWindow(&m_firstWindow);
    childWindow.show();
    auto *childHarmonyWindow = childWindow.nativeInterface<QHarmonyWindow>();

    QVERIFY(childHarmonyWindow);
    QVERIFY(childHarmonyWindow->ability().isNull());
    QVERIFY(childHarmonyWindow->jsWindow().isNull());
    QVERIFY(QTest::qWaitForWindowExposed(&childWindow));
}

void tst_HarmonyWindow::window_abilityAndJsWindow_secondTopLevelWindow()
{
    struct AbilityInfo
    {
        bool abilityIsObject = false;
        bool onWindowStageCreateIsFunction = false;
        bool contextIsObject = false;
        bool getApplicationContextIsFunction = false;
    };

    const auto secondAbilityRef = m_secondHarmonyWindow->ability();
    const auto abilityInfo = QHarmonyApplication::runOnJsThreadAndWait(
        [&](QHarmonyJsContext &jsContext) {
            const auto env = jsContext.env();
            const auto abilityValue = secondAbilityRef.value(jsContext);
            const auto contextValue = getJsProperty(env, abilityValue, "context");
            return AbilityInfo{
                .abilityIsObject = jsTypeOf(env, abilityValue) == napi_object,
                .onWindowStageCreateIsFunction = jsTypeOf(env, getJsProperty(env, abilityValue, "onWindowStageCreate")) == napi_function,
                .contextIsObject = jsTypeOf(env, contextValue) == napi_object,
                .getApplicationContextIsFunction = jsTypeOf(env, getJsProperty(env, contextValue, "getApplicationContext")) == napi_function,
            };
        });
    const auto optFirstJsWindowId = fetchJsWindowId(m_firstHarmonyWindow->jsWindow());
    const auto optSecondJsWindowId = fetchJsWindowId(m_secondHarmonyWindow->jsWindow());

    QVERIFY(!secondAbilityRef.isNull());
    QCOMPARE_NE(secondAbilityRef, m_firstHarmonyWindow->ability());
    QVERIFY(abilityInfo.abilityIsObject);
    QVERIFY(abilityInfo.onWindowStageCreateIsFunction);
    QVERIFY(abilityInfo.contextIsObject);
    QVERIFY(abilityInfo.getApplicationContextIsFunction);
    QVERIFY(optFirstJsWindowId);
    QVERIFY(optSecondJsWindowId);
    QCOMPARE_NE(*optFirstJsWindowId, *optSecondJsWindowId);
}

void tst_HarmonyWindow::window_abilityAndJsWindow_transientWindow()
{
    QWindow transientWindow;
    transientWindow.setTransientParent(&m_secondWindow);
    transientWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&transientWindow));
    auto *transientHarmonyWindow = transientWindow.nativeInterface<QHarmonyWindow>();
    QVERIFY(transientHarmonyWindow);

    const auto optTransientJsWindowId = fetchJsWindowId(transientHarmonyWindow->jsWindow());
    const auto optParentJsWindowId = fetchJsWindowId(m_secondHarmonyWindow->jsWindow());

    QVERIFY(!transientHarmonyWindow->ability().isNull());
    QCOMPARE(transientHarmonyWindow->ability(), m_secondHarmonyWindow->ability());
    QVERIFY(optTransientJsWindowId);
    QVERIFY(optParentJsWindowId);
    QCOMPARE_NE(*optTransientJsWindowId, *optParentJsWindowId);
}

void tst_HarmonyWindow::appJsContext_defaultAbility_unchangedByNewWindow()
{
    const auto defaultAbilityRef = fetchDefaultAbility();

    QWindow newWindow;
    newWindow.show();
    QVERIFY(QTest::qWaitForWindowExposed(&newWindow));
    auto *newHarmonyWindow = newWindow.nativeInterface<QHarmonyWindow>();
    QVERIFY(newHarmonyWindow);
    const auto newAbilityRef = newHarmonyWindow->ability();

    QVERIFY(!defaultAbilityRef.isNull());
    QVERIFY(!newAbilityRef.isNull());
    QCOMPARE_NE(newAbilityRef, defaultAbilityRef);
    QCOMPARE(fetchDefaultAbility(), defaultAbilityRef);
}

QTEST_MAIN(tst_HarmonyWindow)
#include "tst_harmonywindow.moc"
