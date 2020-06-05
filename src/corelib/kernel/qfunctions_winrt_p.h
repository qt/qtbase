/****************************************************************************
**
** Copyright (C) 2020 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QFUNCTIONS_WINRT_P_H
#define QFUNCTIONS_WINRT_P_H

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

#include <QtCore/qglobal.h>

#if defined(Q_OS_WIN) && defined(Q_CC_MSVC)

#include <QtCore/QCoreApplication>
#include <QtCore/QThread>
#include <QtCore/QAbstractEventDispatcher>
#include <QtCore/QElapsedTimer>
#include <QtCore/qt_windows.h>

#include <wrl.h>
#include <windows.foundation.h>

// Convenience macros for handling HRESULT values
#define RETURN_IF_FAILED(msg, ret) \
    if (FAILED(hr)) { \
        qErrnoWarning(hr, msg); \
        ret; \
    }

#define RETURN_IF_FAILED_WITH_ARGS(msg, ret, ...) \
    if (FAILED(hr)) { \
        qErrnoWarning(hr, msg, __VA_ARGS__); \
        ret; \
    }

#define RETURN_HR_IF_FAILED(msg) RETURN_IF_FAILED(msg, return hr)
#define RETURN_OK_IF_FAILED(msg) RETURN_IF_FAILED(msg, return S_OK)
#define RETURN_FALSE_IF_FAILED(msg) RETURN_IF_FAILED(msg, return false)
#define RETURN_VOID_IF_FAILED(msg) RETURN_IF_FAILED(msg, return)
#define RETURN_HR_IF_FAILED_WITH_ARGS(msg, ...) RETURN_IF_FAILED_WITH_ARGS(msg, return hr, __VA_ARGS__)
#define RETURN_OK_IF_FAILED_WITH_ARGS(msg, ...) RETURN_IF_FAILED_WITH_ARGS(msg, return S_OK, __VA_ARGS__)
#define RETURN_FALSE_IF_FAILED_WITH_ARGS(msg, ...) RETURN_IF_FAILED_WITH_ARGS(msg, return false, __VA_ARGS__)
#define RETURN_VOID_IF_FAILED_WITH_ARGS(msg, ...) RETURN_IF_FAILED_WITH_ARGS(msg, return, __VA_ARGS__)

#define Q_ASSERT_SUCCEEDED(hr) \
    Q_ASSERT_X(SUCCEEDED(hr), Q_FUNC_INFO, qPrintable(qt_error_string(hr)));

QT_BEGIN_NAMESPACE

namespace QWinRTFunctions {

// Synchronization methods
enum AwaitStyle
{
    YieldThread = 0,
    ProcessThreadEvents = 1,
    ProcessMainThreadEvents = 2
};

template <typename T>
static inline HRESULT _await_impl(const Microsoft::WRL::ComPtr<T> &asyncOp, AwaitStyle awaitStyle, uint timeout)
{
    Microsoft::WRL::ComPtr<ABI::Windows::Foundation::IAsyncInfo> asyncInfo;
    HRESULT hr = asyncOp.As(&asyncInfo);
    if (FAILED(hr))
        return hr;

    AsyncStatus status;
    QElapsedTimer t;
    if (timeout)
        t.start();
    switch (awaitStyle) {
    case ProcessMainThreadEvents:
        while (SUCCEEDED(hr = asyncInfo->get_Status(&status)) && status == AsyncStatus::Started) {
            QCoreApplication::processEvents();
            if (timeout && t.hasExpired(timeout))
                return ERROR_TIMEOUT;
        }
        break;
    case ProcessThreadEvents:
        if (QAbstractEventDispatcher *dispatcher = QThread::currentThread()->eventDispatcher()) {
            while (SUCCEEDED(hr = asyncInfo->get_Status(&status)) && status == AsyncStatus::Started) {
                dispatcher->processEvents(QEventLoop::AllEvents);
                if (timeout && t.hasExpired(timeout))
                    return ERROR_TIMEOUT;
            }
            break;
        }
        // fall through
    default:
    case YieldThread:
        while (SUCCEEDED(hr = asyncInfo->get_Status(&status)) && status == AsyncStatus::Started) {
            QThread::yieldCurrentThread();
            if (timeout && t.hasExpired(timeout))
                return ERROR_TIMEOUT;
        }
        break;
    }

    if (FAILED(hr) || status != AsyncStatus::Completed) {
        HRESULT ec;
        hr = asyncInfo->get_ErrorCode(&ec);
        if (FAILED(hr))
            return hr;
        hr = asyncInfo->Close();
        if (FAILED(hr))
            return hr;
        return ec;
    }

    return hr;
}

template <typename T>
static inline HRESULT await(const Microsoft::WRL::ComPtr<T> &asyncOp, AwaitStyle awaitStyle = YieldThread, uint timeout = 0)
{
    HRESULT hr = _await_impl(asyncOp, awaitStyle, timeout);
    if (FAILED(hr))
        return hr;

    return asyncOp->GetResults();
}

template <typename T, typename U>
static inline HRESULT await(const Microsoft::WRL::ComPtr<T> &asyncOp, U *results, AwaitStyle awaitStyle = YieldThread, uint timeout = 0)
{
    HRESULT hr = _await_impl(asyncOp, awaitStyle, timeout);
    if (FAILED(hr))
        return hr;

    return asyncOp->GetResults(results);
}

} // QWinRTFunctions

QT_END_NAMESPACE

#endif // Q_OS_WIN && Q_CC_MSVC

#endif // QFUNCTIONS_WINRT_P_H
