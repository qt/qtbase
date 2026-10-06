// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSJSSCOPEDATAOWNER_H
#define QOHOSJSSCOPEDATAOWNER_H

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/private/qohoscommon_p.h>
#include <QtCore/private/qohoslogger_p.h>
#include <exception>
#include <functional>
#include <memory>
#include <qohosplugincore.h>
#include <vector>

QT_BEGIN_NAMESPACE

namespace QtOhos {

// Holds a Qt-thread object's JS-thread state, created on the JS thread on
// first access and released there.
template<typename JsScopeData>
class JsScopeDataOwner
{
public:
    JsScopeDataOwner();
    // not on the JS thread; waits until the data and its cleanups are released
    ~JsScopeDataOwner();

    JsScopeDataOwner(const JsScopeDataOwner &) = delete;
    JsScopeDataOwner(JsScopeDataOwner &&) = delete;
    JsScopeDataOwner &operator=(const JsScopeDataOwner &) = delete;
    JsScopeDataOwner &operator=(JsScopeDataOwner &&) = delete;

    // JS thread only; create the data on first access
    JsScopeData *operator->() const;
    JsScopeData &operator*() const;

    // JS thread only; runs on release in reverse registration order, before the data is destroyed
    void addAsyncCleanup(std::function<void(QOhosJsState &, JsScopeData &, QOhosTaskPromise<>)> cleanupFunc);

private:
    struct State
    {
        std::shared_ptr<JsScopeData> data;
        std::vector<std::function<void(QOhosJsState &, JsScopeData &, QOhosTaskPromise<>)>> asyncCleanupFuncs;
    };

    static void checkJsThreadOrAbort();
    static void runAsyncCleanupsAndDestroyData(
        QOhosJsState &jsState, std::shared_ptr<State> state, std::shared_ptr<QOhosTaskPromise<>> donePromise);
    void createDataIfMissing() const;

    std::shared_ptr<State> m_state;
};

template<typename JsScopeData>
JsScopeDataOwner<JsScopeData>::JsScopeDataOwner()
    : m_state(std::make_shared<State>())
{
}

template<typename JsScopeData>
JsScopeDataOwner<JsScopeData>::~JsScopeDataOwner()
{
    if (isJsThread())
        qOhosReportFatalErrorAndAbort("%s: JS scope data owner destroyed on the JS thread", Q_FUNC_INFO);

    if (!m_state->data)
        return;

    QOhosJsThreadGateway::invokeAndWaitForContinue(
        [state = std::move(m_state)](QOhosJsState &jsState, QOhosTaskPromise<> donePromise) mutable {
            runAsyncCleanupsAndDestroyData(
                jsState, std::move(state), std::make_shared<QOhosTaskPromise<>>(std::move(donePromise)));
        },
        Q_FUNC_INFO);
}

template<typename JsScopeData>
JsScopeData *JsScopeDataOwner<JsScopeData>::operator->() const
{
    checkJsThreadOrAbort();
    createDataIfMissing();
    return m_state->data.get();
}

template<typename JsScopeData>
JsScopeData &JsScopeDataOwner<JsScopeData>::operator*() const
{
    checkJsThreadOrAbort();
    createDataIfMissing();
    return *m_state->data;
}

template<typename JsScopeData>
void JsScopeDataOwner<JsScopeData>::addAsyncCleanup(
    std::function<void(QOhosJsState &, JsScopeData &, QOhosTaskPromise<>)> cleanupFunc)
{
    checkJsThreadOrAbort();
    createDataIfMissing();
    m_state->asyncCleanupFuncs.push_back(std::move(cleanupFunc));
}

template<typename JsScopeData>
void JsScopeDataOwner<JsScopeData>::checkJsThreadOrAbort()
{
    if (!isJsThread())
        qOhosReportFatalErrorAndAbort("%s: JS scope data accessed outside the JS thread", Q_FUNC_INFO);
}

template<typename JsScopeData>
void JsScopeDataOwner<JsScopeData>::createDataIfMissing() const
{
    if (!m_state->data)
        m_state->data = std::make_shared<JsScopeData>();
}

template<typename JsScopeData>
void JsScopeDataOwner<JsScopeData>::runAsyncCleanupsAndDestroyData(
    QOhosJsState &jsState, std::shared_ptr<State> state, std::shared_ptr<QOhosTaskPromise<>> donePromise)
{
    if (state->asyncCleanupFuncs.empty()) {
        state->data.reset();
        (*donePromise)();
        return;
    }

    auto nextCleanupFunc = std::move(state->asyncCleanupFuncs.back());
    state->asyncCleanupFuncs.pop_back();

    auto runNextCleanup = [state, &jsState, donePromise]() {
        runAsyncCleanupsAndDestroyData(jsState, state, donePromise);
    };

    try {
        nextCleanupFunc(jsState, *state->data, QOhosTaskPromise<>(runNextCleanup, runNextCleanup, Q_FUNC_INFO));
    } catch (const std::exception &e) {
        qOhosPrintfError("%s: asynchronous cleanup threw, continuing with the remaining ones: %s", Q_FUNC_INFO, e.what());
    }
}

}

QT_END_NAMESPACE

#endif
