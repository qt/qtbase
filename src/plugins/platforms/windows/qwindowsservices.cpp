// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsservices.h"
#include <QtCore/qt_windows.h>

#include <QtCore/qurl.h>
#include <QtCore/qdebug.h>
#include <QtCore/qdir.h>
#include <QtCore/qscopedpointer.h>
#include <QtCore/qthread.h>

#include <QtCore/private/qwinregistry_p.h>
#include <QtCore/private/qfunctions_win_p.h>

#include <shlobj.h>
#include <shlwapi.h>
#include <intshcut.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

enum { debug = 0 };

class QWindowsShellExecuteThread : public QThread
{
public:
    explicit QWindowsShellExecuteThread(const wchar_t *operation, const wchar_t *file,
                                        const wchar_t *parameters)
        : m_operation(operation)
        , m_file(file)
        , m_parameters(parameters) { }

    void run() override
    {
        QComHelper comHelper;
        if (comHelper.isValid())
            m_result = ShellExecute(nullptr, m_operation, m_file, m_parameters, nullptr,
                                    SW_SHOWNORMAL);
    }

    HINSTANCE result() const { return m_result; }

private:
    HINSTANCE m_result = nullptr;
    const wchar_t *m_operation;
    const wchar_t *m_file;
    const wchar_t *m_parameters;
};

static QString msgShellExecuteFailed(const QUrl &url, quintptr code)
{
    QString result;
    QTextStream(&result) <<"ShellExecute '" <<  url.toString() << "' failed (error " << code << ").";
    return result;
}

// Retrieve the web browser and open the URL. This should be used for URLs with
// fragments which don't work when using ShellExecute() directly (QTBUG-14460,
// QTBUG-55300).
static bool openWebBrowser(const QUrl &url)
{
    WCHAR browserExecutable[MAX_PATH] = {};
    const wchar_t operation[] = L"open";
    DWORD browserExecutableSize = MAX_PATH;
    if (FAILED(AssocQueryString(0, ASSOCSTR_EXECUTABLE, L"http", operation,
                                browserExecutable, &browserExecutableSize))) {
        return false;
    }
    QString browser = QString::fromWCharArray(browserExecutable, browserExecutableSize - 1);
    // Workaround for "old" MS Edge entries. Instead of LaunchWinApp.exe we can just use msedge.exe
    if (browser.contains("LaunchWinApp.exe"_L1, Qt::CaseInsensitive))
        browser = "msedge.exe"_L1;
    const QString urlS = url.toString(QUrl::FullyEncoded);

    // Run ShellExecute() in a thread since it may spin the event loop.
    // Prevent it from interfering with processing of posted events (QTBUG-85676).
    QWindowsShellExecuteThread thread(operation,
                                      reinterpret_cast<const wchar_t *>(browser.utf16()),
                                      reinterpret_cast<const wchar_t *>(urlS.utf16()));
    thread.start();
    thread.wait();

    const auto result = reinterpret_cast<quintptr>(thread.result());
    if (debug)
        qDebug() << __FUNCTION__ << urlS << QString::fromWCharArray(browserExecutable) << result;
    // ShellExecute returns a value greater than 32 if successful
    if (result <= 32) {
        qWarning("%s", qPrintable(msgShellExecuteFailed(url, result)));
        return false;
    }
    return true;
}

static inline bool shellExecute(const QUrl &url)
{
    const QString nativeFilePath = url.isLocalFile() && !url.hasFragment() && !url.hasQuery()
        ? QDir::toNativeSeparators(url.toLocalFile())
        : url.toString(QUrl::FullyEncoded);


    // Run ShellExecute() in a thread since it may spin the event loop.
    // Prevent it from interfering with processing of posted events (QTBUG-85676).
    QWindowsShellExecuteThread thread(nullptr,
                                      reinterpret_cast<const wchar_t *>(nativeFilePath.utf16()),
                                      nullptr);
    thread.start();
    thread.wait();

    const auto result = reinterpret_cast<quintptr>(thread.result());

    // ShellExecute returns a value greater than 32 if successful
    if (result <= 32) {
        qWarning("%s", qPrintable(msgShellExecuteFailed(url, result)));
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

    // Check if user has set preference, otherwise use default.
    QString keyName = QWinRegistryKey(HKEY_CURRENT_USER, mailUserKey)
                      .stringValue( L"Progid");
    const auto mailto = keyName.isEmpty() ? "mailto"_L1 : QLatin1StringView();
    keyName += mailto + "\\Shell\\Open\\Command"_L1;
    if (debug)
        qDebug() << __FUNCTION__ << "keyName=" << keyName;
    const QString command = QWinRegistryKey(HKEY_CLASSES_ROOT, keyName).stringValue(L"");
    // QTBUG-57816: As of Windows 10, if there is no mail client installed, an entry like
    // "rundll32.exe .. url.dll,MailToProtocolHandler %l" is returned. Launching it
    // silently fails or brings up a broken dialog after a long time, so exclude it and
    // fall back to ShellExecute() which brings up the URL association dialog.
    if (command.isEmpty() || command.contains(u",MailToProtocolHandler"))
        return QString();
    wchar_t expandedCommand[MAX_PATH] = {0};
    return ExpandEnvironmentStrings(reinterpret_cast<const wchar_t *>(command.utf16()),
                                    expandedCommand, MAX_PATH)
        ? QString::fromWCharArray(expandedCommand) : command;
}

static inline bool launchMail(const QUrl &url)
{
    QString command = mailCommand();
    if (command.isEmpty()) {
        qWarning("Cannot launch '%ls': There is no mail program installed.", qUtf16Printable(url.toString()));
        return false;
    }
    // Fix mail launch if no param is expected in this command.
    if (command.indexOf(QStringLiteral("%1")) < 0) {
        qWarning() << "The mail command lacks the '%1' parameter.";
        return false;
    }
    //Make sure the path for the process is in quotes
    const QChar doubleQuote = u'"';
    if (!command.startsWith(doubleQuote)) {
        const int exeIndex = command.indexOf(QStringLiteral(".exe "), 0, Qt::CaseInsensitive);
        if (exeIndex != -1) {
            command.insert(exeIndex + 4, doubleQuote);
            command.prepend(doubleQuote);
        }
    }
    // Pass the url as the parameter. Should use QProcess::startDetached(),
    // but that cannot handle a Windows command line [yet].
    command.replace("%1"_L1, url.toString(QUrl::FullyEncoded));
    if (debug)
        qDebug() << __FUNCTION__ << "Launching" << command;
    //start the process
    PROCESS_INFORMATION pi;
    ZeroMemory(&pi, sizeof(pi));
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    if (!CreateProcess(nullptr, reinterpret_cast<wchar_t *>(const_cast<ushort *>(command.utf16())),
                       nullptr, nullptr, FALSE, 0, nullptr, nullptr, &si, &pi)) {
        qErrnoWarning("Unable to launch '%ls'", qUtf16Printable(command));
        return false;
    }
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    return true;
}

bool QWindowsServices::openUrl(const QUrl &url)
{
    const QString scheme = url.scheme();
    if (scheme == u"mailto" && launchMail(url))
        return true;
    return url.isLocalFile() && url.hasFragment()
        ? openWebBrowser(url) : shellExecute(url);
}

bool QWindowsServices::openDocument(const QUrl &url)
{
    return shellExecute(url);
}

QT_END_NAMESPACE
