// Copyright (C) 2026 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qohosmemoizingjsthreadfetcher_p.h"
#include <QtCore/private/qohoslogger_p.h>
#include <exception>

QT_BEGIN_NAMESPACE

namespace details_qohosmemoizingjsthreadfetcher_p_h {

QBasicMutex FetcherBase::s_fetchersMutex;
FetcherBase *FetcherBase::s_firstFetcher = nullptr;
std::atomic<bool> FetcherBase::s_fetchingEnabled{false};

FetcherBase::FetcherBase(const char *fetcherName)
    : m_fetcherName(fetcherName)
{
}

FetcherBase::~FetcherBase() = default;

void FetcherBase::registerSelf()
{
    QMutexLocker fetchersLock{&s_fetchersMutex};
    m_nextFetcher = s_firstFetcher;
    s_firstFetcher = this;
}

void FetcherBase::unregisterSelf()
{
    QMutexLocker fetchersLock{&s_fetchersMutex};
    for (auto **fetcherPtr = &s_firstFetcher; *fetcherPtr; fetcherPtr = &(*fetcherPtr)->m_nextFetcher) {
        if (*fetcherPtr == this) {
            *fetcherPtr = m_nextFetcher;
            return;
        }
    }
}

void FetcherBase::enableFetching(QOhosJsState &jsState)
{
    fetchPendingFetchers(jsState);
    s_fetchingEnabled.store(true);
}

void FetcherBase::ensureFetched()
{
    if (!s_fetchingEnabled.load() || m_fetched.load())
        return;

    QOhosJsThreadGateway::runAndWait(
        [](QOhosJsState &jsState) {
            fetchPendingFetchers(jsState);
        },
        Q_FUNC_INFO);
}

void FetcherBase::fetchPendingFetchers(QOhosJsState &jsState)
{
    QMutexLocker fetchersLock{&s_fetchersMutex};
    for (auto *fetcher = s_firstFetcher; fetcher; fetcher = fetcher->m_nextFetcher) {
        if (fetcher->m_fetched.load())
            continue;

        try {
            fetcher->fetchInJsThread(jsState);
        } catch (const std::exception &error) {
            qOhosPrintfError(
                "%s: failed to fetch '%s' from the JS thread: %s",
                Q_FUNC_INFO, fetcher->m_fetcherName, error.what());
        }
        fetcher->m_fetched.store(true);
    }
}

}

void qOhosEnableMemoizingJsThreadFetchers(QOhosJsState &jsState)
{
    details_qohosmemoizingjsthreadfetcher_p_h::FetcherBase::enableFetching(jsState);
}

QT_END_NAMESPACE
