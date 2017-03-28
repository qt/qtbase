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

#define QT_NO_URL_CAST_FROM_STRING
#include "qwindowsservices.h"
#include <QtCore/qt_windows.h>

#include <QtCore/QUrl>
#include <QtCore/QDebug>
#include <QtCore/QDir>

#include <shlobj.h>
#include <intshcut.h>

QT_BEGIN_NAMESPACE

enum { debug = 0 };

static inline bool shellExecute(const QUrl &url)
{
    const QString nativeFilePath = url.isLocalFile() && !url.hasFragment() && !url.hasQuery()
        ? QDir::toNativeSeparators(url.toLocalFile())
        : url.toString(QUrl::FullyEncoded);
    const quintptr result =
        reinterpret_cast<quintptr>(ShellExecute(0, 0,
                                                reinterpret_cast<const wchar_t *>(nativeFilePath.utf16()),
                                                0, 0, SW_SHOWNORMAL));
    // ShellExecute returns a value greater than 32 if successful
    if (result <= 32) {
        qWarning("ShellExecute '%s' failed (error %s).", qPrintable(url.toString()), qPrintable(QString::number(result)));
        return false;
    }
    return true;
}

// Retrieve the commandline for the default mail client. It contains a
// placeholder %1 for the URL. The default key used below is the
// command line for the mailto: shell command.
static inline QString mailCommand()
{
    enum { BufferSize = sizeof(wchar_t) * MAX_PATH };

    const wchar_t mailUserKey[] = L"Software\\Microsoft\\Windows\\Shell\\Associations\\UrlAssociations\\mailto\\UserChoice";

    wchar_t command[MAX_PATH] = {0};
    // Check if user has set preference, otherwise use default.
    HKEY handle;
    QString keyName;
    if (!RegOpenKeyEx(HKEY_CURRENT_USER, mailUserKey, 0, KEY_READ, &handle)) {
        DWORD bufferSize = BufferSize;
        if (!RegQueryValueEx(handle, L"Progid", 0, 0, reinterpret_cast<unsigned char*>(command), &bufferSize))
            keyName = QString::fromWCharArray(command);
        RegCloseKey(handle);
    }
    const QLatin1String mailto = keyName.isEmpty() ? QLatin1String("mailto") : QLatin1String();
    keyName += mailto + QLatin1String("\\Shell\\Open\\Command");
    if (debug)
        qDebug() << __FUNCTION__ << "keyName=" << keyName;
    command[0] = 0;
    if (!RegOpenKeyExW(HKEY_CLASSES_ROOT, reinterpret_cast<const wchar_t*>(keyName.utf16()), 0, KEY_READ, &handle)) {
        DWORD bufferSize = BufferSize;
        RegQueryValueEx(handle, L"", 0, 0, reinterpret_cast<unsigned char*>(command), &bufferSize);
        RegCloseKey(handle);
    }
    // QTBUG-57816: As of Windows 10, if there is no mail client installed, an entry like
    // "rundll32.exe .. url.dll,MailToProtocolHandler %l" is returned. Launching it
    // silently fails or brings up a broken dialog after a long time, so exclude it and
    // fall back to ShellExecute() which brings up the URL assocation dialog.
    if (!command[0] || wcsstr(command, L",MailToProtocolHandler") != nullptr)
        return QString();
    wchar_t expandedCommand[MAX_PATH] = {0};
    return ExpandEnvironmentStrings(command, expandedCommand, MAX_PATH) ?
           QString::fromWCharArray(expandedCommand) : QString::fromWCharArray(command);
}

static inline bool launchMail(const QUrl &url)
{
    QString command = mailCommand();
    if (command.isEmpty()) {
        qWarning("Cannot launch '%s': There is no mail program installed.", qPrintable(url.toString()));
        return false;
    }
    //Make sure the path for the process is in quotes
    const QChar doubleQuote = QLatin1Char('"');
    if (!command.startsWith(doubleQuote)) {
        const int exeIndex = command.indexOf(QStringLiteral(".exe "), 0, Qt::CaseInsensitive);
        if (exeIndex != -1) {
            command.insert(exeIndex + 4, doubleQuote);
            command.prepend(doubleQuote);
        }
    }
    // Pass the url as the parameter. Should use QProcess::startDetached(),
    // but that cannot handle a Windows command line [yet].
    command.replace(QLatin1String("%1"), url.toString(QUrl::FullyEncoded));
    if (debug)
        qDebug() << __FUNCTION__ << "Launching" << command;
    //start the process
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    if (!CreateProcess(NULL, reinterpret_cast<wchar_t *>(const_cast<ushort *>(command.utf16())),
                       NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi)) {
        qErrnoWarning("Unable to launch '%s'", qPrintable(command));
        return false;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

bool QWindowsServices::openUrl(const QUrl &url)
{
    const QString scheme = url.scheme();
    if (scheme == QLatin1String("mailto") && launchMail(url))
        return true;
    return shellExecute(url);
}

bool QWindowsServices::openDocument(const QUrl &url)
{
    return shellExecute(url);
}

QT_END_NAMESPACE
