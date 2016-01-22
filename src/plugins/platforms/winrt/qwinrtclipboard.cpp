/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qwinrtclipboard.h"

#include <QtCore/QCoreApplication>
#include <QtCore/qfunctions_winrt.h>
#include <QtCore/private/qeventdispatcher_winrt_p.h>

#include <Windows.ApplicationModel.datatransfer.h>

#include <functional>

using namespace ABI::Windows::ApplicationModel::DataTransfer;
using namespace ABI::Windows::Foundation;
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;

typedef IEventHandler<IInspectable *> ContentChangedHandler;

#define RETURN_NULLPTR_IF_FAILED(msg) RETURN_IF_FAILED(msg, return nullptr)

QT_BEGIN_NAMESPACE

QWinRTClipboard::QWinRTClipboard()
{
#ifndef Q_OS_WINPHONE
    QEventDispatcherWinRT::runOnXamlThread([this]() {
        HRESULT hr;
        hr = GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_ApplicationModel_DataTransfer_Clipboard).Get(),
                                  &m_nativeClipBoard);
        Q_ASSERT_SUCCEEDED(hr);

        EventRegistrationToken tok;
        hr = m_nativeClipBoard->add_ContentChanged(Callback<ContentChangedHandler>(this, &QWinRTClipboard::onContentChanged).Get(), &tok);
        Q_ASSERT_SUCCEEDED(hr);

        return hr;
    });
#endif // !Q_OS_WINPHONE
}

QMimeData *QWinRTClipboard::mimeData(QClipboard::Mode mode)
{
    if (!supportsMode(mode))
        return nullptr;

#ifndef Q_OS_WINPHONE
    ComPtr<IDataPackageView> view;
    HRESULT hr;
    hr = m_nativeClipBoard->GetContent(&view);
    RETURN_NULLPTR_IF_FAILED("Could not get clipboard content.");

    ComPtr<IAsyncOperation<HSTRING>> op;
    HString result;
    // This throws a security exception (WinRT originate error / 0x40080201.
    // Unfortunately there seems to be no way to avoid this, neither
    // running on the XAML thread, nor some other way. Stack Overflow
    // confirms this problem since Windows (Phone) 8.0.
    hr = view->GetTextAsync(&op);
    RETURN_NULLPTR_IF_FAILED("Could not get clipboard text.");

    hr = QWinRTFunctions::await(op, result.GetAddressOf());
    RETURN_NULLPTR_IF_FAILED("Could not get clipboard text content");

    quint32 size;
    const wchar_t *textStr = result.GetRawBuffer(&size);
    QString text = QString::fromWCharArray(textStr, size);
    text.replace(QStringLiteral("\r\n"), QStringLiteral("\n"));
    m_mimeData.setText(text);

    return &m_mimeData;
#else // Q_OS_WINPHONE
    return QPlatformClipboard::mimeData(mode);
#endif // Q_OS_WINPHONE
}

// Inspired by QWindowsMimeText::convertFromMime
inline QString convertToWindowsLineEnding(const QString &text)
{
    const QChar *u = text.unicode();
    QString res;
    const int s = text.length();
    int maxsize = s + s / 40 + 3;
    res.resize(maxsize);
    int ri = 0;
    bool cr = false;
    for (int i = 0; i < s; ++i) {
        if (*u == QLatin1Char('\r'))
            cr = true;
        else {
            if (*u == QLatin1Char('\n') && !cr)
                res[ri++] = QLatin1Char('\r');
            cr = false;
        }
        res[ri++] = *u;
        if (ri+3 >= maxsize) {
            maxsize += maxsize / 4;
            res.resize(maxsize);
        }
        ++u;
    }
    res.truncate(ri);
    return res;
}

void QWinRTClipboard::setMimeData(QMimeData *data, QClipboard::Mode mode)
{
    if (!supportsMode(mode))
        return;

#ifndef Q_OS_WINPHONE
    const QString text = data->text();
    HRESULT hr = QEventDispatcherWinRT::runOnXamlThread([this, text]() {
        HRESULT hr;
        ComPtr<IDataPackage> package;
        hr = RoActivateInstance(HString::MakeReference(RuntimeClass_Windows_ApplicationModel_DataTransfer_DataPackage).Get(),
                                &package);

        const QString nativeString = convertToWindowsLineEnding(text);
        HStringReference textRef(reinterpret_cast<LPCWSTR>(nativeString.utf16()), nativeString.length());

        hr = package->SetText(textRef.Get());
        RETURN_HR_IF_FAILED("Could not set text to clipboard data package.");

        hr = m_nativeClipBoard->SetContent(package.Get());
        RETURN_HR_IF_FAILED("Could not set clipboard content.");
        return S_OK;
    });
    RETURN_VOID_IF_FAILED("Could not set clipboard text.");
    emitChanged(mode);
#else // Q_OS_WINPHONE
    QPlatformClipboard::setMimeData(data, mode);
#endif // Q_OS_WINPHONE
}

bool QWinRTClipboard::supportsMode(QClipboard::Mode mode) const
{
#ifndef Q_OS_WINPHONE
    return mode == QClipboard::Clipboard;
#else
    return QPlatformClipboard::supportsMode(mode);
#endif
}

HRESULT QWinRTClipboard::onContentChanged(IInspectable *, IInspectable *)
{
    emitChanged(QClipboard::Clipboard);
    return S_OK;
}

QT_END_NAMESPACE
