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

#include "qwinrtservices.h"
#include "qwinrtfileengine.h"
#include <QtCore/QUrl>
#include <QtCore/QDir>
#include <QtCore/QCoreApplication>
#include <QtCore/qfunctions_winrt.h>
#include <private/qeventdispatcher_winrt_p.h>

#include <wrl.h>
#include <windows.foundation.h>
#include <windows.storage.h>
#include <windows.system.h>
using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::System;

QT_BEGIN_NAMESPACE

class QWinRTServicesPrivate
{
public:
    ComPtr<IUriRuntimeClassFactory> uriFactory;
    ComPtr<IStorageFileStatics> fileFactory;
    ComPtr<ILauncherStatics> launcher;
};

QWinRTServices::QWinRTServices()
    : d_ptr(new QWinRTServicesPrivate)
{
    Q_D(QWinRTServices);

    HRESULT hr;
    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Foundation_Uri).Get(),
                                IID_PPV_ARGS(&d->uriFactory));
    Q_ASSERT_X(SUCCEEDED(hr), Q_FUNC_INFO, qPrintable(qt_error_string(hr)));

    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_StorageFile).Get(),
                                IID_PPV_ARGS(&d->fileFactory));
    Q_ASSERT_X(SUCCEEDED(hr), Q_FUNC_INFO, qPrintable(qt_error_string(hr)));

    hr = RoGetActivationFactory(HString::MakeReference(RuntimeClass_Windows_System_Launcher).Get(),
                                IID_PPV_ARGS(&d->launcher));
    Q_ASSERT_X(SUCCEEDED(hr), Q_FUNC_INFO, qPrintable(qt_error_string(hr)));
}

bool QWinRTServices::openUrl(const QUrl &url)
{
    Q_D(QWinRTServices);

    ComPtr<IUriRuntimeClass> uri;
    QString urlString = url.toString();
    HStringReference uriString(reinterpret_cast<LPCWSTR>(urlString.utf16()),
                               uint(urlString.length()));
    HRESULT hr = d->uriFactory->CreateUri(uriString.Get(), &uri);
    RETURN_FALSE_IF_FAILED("Failed to create URI from QUrl.");

    boolean result;
    hr = QEventDispatcherWinRT::runOnXamlThread([this, d, uri, &result]() {
        ComPtr<IAsyncOperation<bool>> op;
        HRESULT hr = d->launcher->LaunchUriAsync(uri.Get(), &op);
        RETURN_HR_IF_FAILED("Failed to start URI launch.");

        hr = QWinRTFunctions::await(op, &result);
        RETURN_HR_IF_FAILED("Failed to launch URI.");
        return hr;
    });
    RETURN_FALSE_IF_FAILED("Failed to launch URI from Xaml thread.");

    return result;
}

bool QWinRTServices::openDocument(const QUrl &url)
{
    Q_D(QWinRTServices);

    HRESULT hr;
    ComPtr<IStorageFile> file;
    ComPtr<IStorageItem> item = QWinRTFileEngineHandler::registeredFile(url.toLocalFile());
    if (item) {
        hr = item.As(&file);
        if (FAILED(hr))
            qErrnoWarning(hr, "Failed to cast picked item to a file");
    }
    if (!file) {
        const QString pathString = QDir::toNativeSeparators(url.toLocalFile());
        HStringReference path(reinterpret_cast<LPCWSTR>(pathString.utf16()),
                              uint(pathString.length()));
        ComPtr<IAsyncOperation<StorageFile *>> op;
        hr = d->fileFactory->GetFileFromPathAsync(path.Get(), &op);
        RETURN_FALSE_IF_FAILED("Failed to initialize file URI.");

        hr = QWinRTFunctions::await(op, file.GetAddressOf());
        RETURN_FALSE_IF_FAILED("Failed to get file URI.");
    }

    boolean result;
    {
        hr = QEventDispatcherWinRT::runOnXamlThread([this, d, file, &result]() {
            ComPtr<IAsyncOperation<bool>> op;
            HRESULT hr = d->launcher->LaunchFileAsync(file.Get(), &op);
            RETURN_HR_IF_FAILED("Failed to start file launch.");

            hr = QWinRTFunctions::await(op, &result);
            RETURN_HR_IF_FAILED("Failed to launch file.");
            return hr;
        });
        RETURN_FALSE_IF_FAILED("Failed to launch file from Xaml thread.");
    }

    return result;
}

QT_END_NAMESPACE
