// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSMEMOIZINGJSTHREADFETCHER_P_H
#define QOHOSMEMOIZINGJSTHREADFETCHER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <QtCore/private/qcore_ohos_p.h>
#include <QtCore/qmutex.h>
#include <atomic>
#include <optional>
#include <type_traits>

QT_BEGIN_NAMESPACE

// must be called from the JS thread, once the JS thread gateway is registered
Q_CORE_EXPORT void qOhosEnableMemoizingJsThreadFetchers(QOhosJsState &jsState);

namespace details_qohosmemoizingjsthreadfetcher_p_h {

class Q_CORE_EXPORT FetcherBase
{
public:
    FetcherBase(const FetcherBase &) = delete;
    FetcherBase &operator=(const FetcherBase &) = delete;

    static void enableFetching(QOhosJsState &jsState);

protected:
    explicit FetcherBase(const char *fetcherName);
    virtual ~FetcherBase();

    // must be called at the end of the most derived constructor
    void registerSelf();
    // must be called at the beginning of the most derived destructor
    void unregisterSelf();

    // may be called from any thread
    void ensureFetched();

private:
    virtual void fetchInJsThread(QOhosJsState &jsState) = 0;

    static void fetchPendingFetchers(QOhosJsState &jsState);

    static QBasicMutex s_fetchersMutex;
    static FetcherBase *s_firstFetcher;
    static std::atomic<bool> s_fetchingEnabled;

    const char *m_fetcherName;
    FetcherBase *m_nextFetcher = nullptr;
    std::atomic<bool> m_fetched{false};
};

}

template<typename T>
class QOhosMemoizingJsThreadFetcher final : public details_qohosmemoizingjsthreadfetcher_p_h::FetcherBase
{
public:
    static_assert(
        !std::is_convertible<T, napi_value>::value && !std::is_convertible<T, napi_ref>::value,
        "NAPI values/references must not be accessed outside the JS thread");

    // Contract for fetchFunc:
    //  - it must return a value that never changes for the lifetime of the process, because it is read once and never refreshed
    //  - it runs in the JS thread, and never runs at all in a process that has no JS thread
    //  - it may run at any time after construction, not only at this fetcher's first optValue(), so it must not depend on state initialized later in the application startup
    //  - it must not read another fetcher
    //  - it must be cheap: its cost is paid by whoever triggers the batch it lands in
    //  - it may return nullopt, and any outcome, including an escaping exception, is final and never retried
    // fetcherName is stored by pointer and must outlive the fetcher, so pass a string literal
    QOhosMemoizingJsThreadFetcher(
        std::optional<T> (*fetchFunc)(QOhosJsState &), const char *fetcherName);
    ~QOhosMemoizingJsThreadFetcher();

    // the first call may block until the JS thread runs the fetch function
    std::optional<T> optValue();

private:
    void fetchInJsThread(QOhosJsState &jsState) override;

    std::optional<T> (*m_fetchFunc)(QOhosJsState &);
    std::optional<T> m_optValue;
};

template<typename T>
QOhosMemoizingJsThreadFetcher<T>::QOhosMemoizingJsThreadFetcher(
    std::optional<T> (*fetchFunc)(QOhosJsState &), const char *fetcherName)
    : FetcherBase(fetcherName), m_fetchFunc(fetchFunc)
{
    registerSelf();
}

template<typename T>
QOhosMemoizingJsThreadFetcher<T>::~QOhosMemoizingJsThreadFetcher()
{
    unregisterSelf();
}

template<typename T>
std::optional<T> QOhosMemoizingJsThreadFetcher<T>::optValue()
{
    ensureFetched();
    return m_optValue;
}

template<typename T>
void QOhosMemoizingJsThreadFetcher<T>::fetchInJsThread(QOhosJsState &jsState)
{
    m_optValue = m_fetchFunc(jsState);
}

QT_END_NAMESPACE

#endif // QOHOSMEMOIZINGJSTHREADFETCHER_P_H
