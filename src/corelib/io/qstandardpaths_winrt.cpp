/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
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

#include "qstandardpaths.h"

#include <qdir.h>
#include <private/qsystemlibrary_p.h>
#include <qcoreapplication.h>
#include <qstringlist.h>

#include <qt_windows.h>

#include <wrl.h>
#include <windows.foundation.h>
#include <windows.storage.h>
#include <Windows.ApplicationModel.h>

using namespace Microsoft::WRL;
using namespace Microsoft::WRL::Wrappers;
using namespace ABI::Windows::Foundation;
using namespace ABI::Windows::Storage;
using namespace ABI::Windows::ApplicationModel;

#ifndef QT_NO_STANDARDPATHS

QT_BEGIN_NAMESPACE

static QString convertCharArray(const wchar_t *path)
{
    return QDir::fromNativeSeparators(QString::fromWCharArray(path));
}

QString QStandardPaths::writableLocation(StandardLocation type)
{
    QString result;

    switch (type) {
    case ConfigLocation: // same as AppLocalDataLocation, on Windows
    case GenericConfigLocation: // same as GenericDataLocation, on Windows
    case AppConfigLocation:
    case AppDataLocation:
    case AppLocalDataLocation:
    case GenericDataLocation: {
        ComPtr<IApplicationDataStatics> applicationDataStatics;
        if (FAILED(GetActivationFactory(HString::MakeReference(RuntimeClass_Windows_Storage_ApplicationData).Get(), &applicationDataStatics)))
            break;
        ComPtr<IApplicationData> applicationData;
        if (FAILED(applicationDataStatics->get_Current(&applicationData)))
            break;
        ComPtr<IStorageFolder> settingsFolder;
        if (FAILED(applicationData->get_LocalFolder(&settingsFolder)))
            break;
        ComPtr<IStorageItem> settingsFolderItem;
        if (FAILED(settingsFolder.As(&settingsFolderItem)))
            break;
        HString path;
        if (FAILED(settingsFolderItem->get_Path(path.GetAddressOf())))
            break;
        result = convertCharArray(path.GetRawBuffer(nullptr));
        if (isTestModeEnabled())
            result += QLatin1String("/qttest");
        break;
    }
    case CacheLocation:
        return writableLocation(AppLocalDataLocation) + QLatin1String("/cache");

    case GenericCacheLocation:
        return writableLocation(GenericDataLocation) + QLatin1String("/cache");

    case TempLocation:
        result = QDir::tempPath();
        break;

    case ApplicationsLocation:
    case DesktopLocation:
    case FontsLocation:
    case HomeLocation:
    case RuntimeLocation:
        // these are read-only
        break;

    case DocumentsLocation:
    case MusicLocation:
    case MoviesLocation:
    case PicturesLocation:
    case DownloadLocation:
    default:
        Q_UNIMPLEMENTED();
    }
    return result;

}

QStringList QStandardPaths::standardLocations(StandardLocation type)
{
    const QString writable = writableLocation(type);
    return writable.isEmpty() ? QStringList() : QStringList(writable);
}

QT_END_NAMESPACE

#endif // QT_NO_STANDARDPATHS
