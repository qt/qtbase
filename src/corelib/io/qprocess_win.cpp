/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2017 Intel Corporation.
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

//#define QPROCESS_DEBUG
#include <qdebug.h>
#include <private/qdebug_p.h>

#include "qprocess.h"
#include "qprocess_p.h"
#include "qwindowspipereader_p.h"
#include "qwindowspipewriter_p.h"

#include <qdatetime.h>
#include <qdir.h>
#include <qfileinfo.h>
#include <qrandom.h>
#include <qwineventnotifier.h>
#include <qscopedvaluerollback.h>
#include <qtimer.h>
#include <private/qsystemlibrary_p.h>
#include <private/qthread_p.h>

#include "private/qfsfileengine_p.h" // for longFileName

#ifndef PIPE_REJECT_REMOTE_CLIENTS
#define PIPE_REJECT_REMOTE_CLIENTS 0x08
#endif

QT_BEGIN_NAMESPACE

QProcessEnvironment QProcessEnvironment::systemEnvironment()
{
    QProcessEnvironment env;
    // Calls to setenv() affect the low-level environment as well.
    // This is not the case the other way round.
    if (wchar_t *envStrings = GetEnvironmentStringsW()) {
        for (const wchar_t *entry = envStrings; *entry; ) {
            const int entryLen = int(wcslen(entry));
            // + 1 to permit magic cmd variable names starting with =
            if (const wchar_t *equal = wcschr(entry + 1, L'=')) {
                int nameLen = equal - entry;
                QString name = QString::fromWCharArray(entry, nameLen);
                QString value = QString::fromWCharArray(equal + 1, entryLen - nameLen - 1);
                env.d->vars.insert(QProcessEnvironmentPrivate::Key(name), value);
            }
            entry += entryLen + 1;
        }
        FreeEnvironmentStringsW(envStrings);
    }
    return env;
}

#if QT_CONFIG(process)

namespace {
struct QProcessPoller
{
    QProcessPoller(const QProcessPrivate &proc);

    int poll(const QDeadlineTimer &deadline);

    enum { maxHandles = 4 };
    HANDLE handles[maxHandles];
    DWORD handleCount = 0;
};

QProcessPoller::QProcessPoller(const QProcessPrivate &proc)
{
    if (proc.stdinChannel.writer)
        handles[handleCount++] = proc.stdinChannel.writer->syncEvent();
    if (proc.stdoutChannel.reader)
        handles[handleCount++] = proc.stdoutChannel.reader->syncEvent();
    if (proc.stderrChannel.reader)
        handles[handleCount++] = proc.stderrChannel.reader->syncEvent();

    handles[handleCount++] = proc.pid->hProcess;
}

int QProcessPoller::poll(const QDeadlineTimer &deadline)
{
    DWORD waitRet;

    do {
        waitRet = WaitForMultipleObjectsEx(handleCount, handles, FALSE,
                                           deadline.remainingTime(), TRUE);
    } while (waitRet == WAIT_IO_COMPLETION);

    if (waitRet - WAIT_OBJECT_0 < handleCount)
        return 1;

    return (waitRet == WAIT_TIMEOUT) ? 0 : -1;
}
} // anonymous namespace

static bool qt_create_pipe(Q_PIPE *pipe, bool isInputPipe, BOOL defInheritFlag)
{
    // Anomymous pipes do not support asynchronous I/O. Thus we
    // create named pipes for redirecting stdout, stderr and stdin.

    // The write handle must be non-inheritable for input pipes.
    // The read handle must be non-inheritable for output pipes.
    // When one process pipes to another (setStandardOutputProcess() was called),
    // both handles must be inheritable (defInheritFlag == TRUE).
    SECURITY_ATTRIBUTES secAtt = { sizeof(SECURITY_ATTRIBUTES), 0, defInheritFlag };

    HANDLE hServer;
    wchar_t pipeName[256];
    unsigned int attempts = 1000;
    forever {
        _snwprintf(pipeName, sizeof(pipeName) / sizeof(pipeName[0]),
                L"\\\\.\\pipe\\qt-%lX-%X", long(QCoreApplication::applicationPid()),
                QRandomGenerator::global()->generate());

        DWORD dwOpenMode = FILE_FLAG_OVERLAPPED;
        DWORD dwOutputBufferSize = 0;
        DWORD dwInputBufferSize = 0;
        const DWORD dwPipeBufferSize = 1024 * 1024;
        if (isInputPipe) {
            dwOpenMode |= PIPE_ACCESS_OUTBOUND;
            dwOutputBufferSize = dwPipeBufferSize;
        } else {
            dwOpenMode |= PIPE_ACCESS_INBOUND;
            dwInputBufferSize = dwPipeBufferSize;
        }
        DWORD dwPipeFlags = PIPE_TYPE_BYTE | PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS;
        hServer = CreateNamedPipe(pipeName,
                                  dwOpenMode,
                                  dwPipeFlags,
                                  1,                      // only one pipe instance
                                  dwOutputBufferSize,
                                  dwInputBufferSize,
                                  0,
                                  &secAtt);
        if (hServer != INVALID_HANDLE_VALUE)
            break;
        DWORD dwError = GetLastError();
        if (dwError != ERROR_PIPE_BUSY || !--attempts) {
            qErrnoWarning(dwError, "QProcess: CreateNamedPipe failed.");
            return false;
        }
    }

    secAtt.bInheritHandle = TRUE;
    const HANDLE hClient = CreateFile(pipeName,
                                      (isInputPipe ? (GENERIC_READ | FILE_WRITE_ATTRIBUTES)
                                                   : GENERIC_WRITE),
                                      0,
                                      &secAtt,
                                      OPEN_EXISTING,
                                      FILE_FLAG_OVERLAPPED,
                                      NULL);
    if (hClient == INVALID_HANDLE_VALUE) {
        qErrnoWarning("QProcess: CreateFile failed.");
        CloseHandle(hServer);
        return false;
    }

    // Wait until connection is in place.
    OVERLAPPED overlapped;
    ZeroMemory(&overlapped, sizeof(overlapped));
    overlapped.hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    if (ConnectNamedPipe(hServer, &overlapped) == 0) {
        DWORD dwError = GetLastError();
        switch (dwError) {
        case ERROR_PIPE_CONNECTED:
            break;
        case ERROR_IO_PENDING:
            WaitForSingleObject(overlapped.hEvent, INFINITE);
            break;
        default:
            qErrnoWarning(dwError, "QProcess: ConnectNamedPipe failed.");
            CloseHandle(overlapped.hEvent);
            CloseHandle(hClient);
            CloseHandle(hServer);
            return false;
        }
    }
    CloseHandle(overlapped.hEvent);

    if (isInputPipe) {
        pipe[0] = hClient;
        pipe[1] = hServer;
    } else {
        pipe[0] = hServer;
        pipe[1] = hClient;
    }
    return true;
}

/*
    Create the pipes to a QProcessPrivate::Channel.
*/
bool QProcessPrivate::openChannel(Channel &channel)
{
    Q_Q(QProcess);

    switch (channel.type) {
    case Channel::Normal: {
        // we're piping this channel to our own process
        if (&channel == &stdinChannel)
            return qt_create_pipe(channel.pipe, true, FALSE);

        if (&channel == &stdoutChannel) {
            if (!stdoutChannel.reader) {
                stdoutChannel.reader = new QWindowsPipeReader(q);
                q->connect(stdoutChannel.reader, SIGNAL(readyRead()), SLOT(_q_canReadStandardOutput()));
            }
        } else /* if (&channel == &stderrChannel) */ {
            if (!stderrChannel.reader) {
                stderrChannel.reader = new QWindowsPipeReader(q);
                q->connect(stderrChannel.reader, SIGNAL(readyRead()), SLOT(_q_canReadStandardError()));
            }
        }
        if (!qt_create_pipe(channel.pipe, false, FALSE))
            return false;

        channel.reader->setHandle(channel.pipe[0]);
        channel.reader->startAsyncRead();
        return true;
    }
    case Channel::Redirect: {
        // we're redirecting the channel to/from a file
        SECURITY_ATTRIBUTES secAtt = { sizeof(SECURITY_ATTRIBUTES), NULL, TRUE };

        if (&channel == &stdinChannel) {
            // try to open in read-only mode
            channel.pipe[1] = INVALID_Q_PIPE;
            channel.pipe[0] =
                CreateFile((const wchar_t*)QFSFileEnginePrivate::longFileName(channel.file).utf16(),
                           GENERIC_READ,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           &secAtt,
                           OPEN_EXISTING,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

            if (channel.pipe[0] != INVALID_Q_PIPE)
                return true;

            setErrorAndEmit(QProcess::FailedToStart,
                            QProcess::tr("Could not open input redirection for reading"));
        } else {
            // open in write mode
            channel.pipe[0] = INVALID_Q_PIPE;
            channel.pipe[1] =
                CreateFile((const wchar_t *)QFSFileEnginePrivate::longFileName(channel.file).utf16(),
                           GENERIC_WRITE,
                           FILE_SHARE_READ | FILE_SHARE_WRITE,
                           &secAtt,
                           channel.append ? OPEN_ALWAYS : CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,
                           NULL);

            if (channel.pipe[1] != INVALID_Q_PIPE) {
                if (channel.append) {
                    SetFilePointer(channel.pipe[1], 0, NULL, FILE_END);
                }
                return true;
            }

            setErrorAndEmit(QProcess::FailedToStart,
                            QProcess::tr("Could not open output redirection for writing"));
        }
        cleanup();
        return false;
    }
    case Channel::PipeSource: {
        Q_ASSERT_X(channel.process, "QProcess::start", "Internal error");
        // we are the source
        Channel *source = &channel;
        Channel *sink = &channel.process->stdinChannel;

        if (source->pipe[1] != INVALID_Q_PIPE) {
            // already constructed by the sink
            return true;
        }

        Q_ASSERT(source == &stdoutChannel);
        Q_ASSERT(sink->process == this && sink->type == Channel::PipeSink);

        if (!qt_create_pipe(source->pipe, /* in = */ false, TRUE))  // source is stdout
            return false;

        sink->pipe[0] = source->pipe[0];
        source->pipe[0] = INVALID_Q_PIPE;

        return true;
    }
    case Channel::PipeSink: { // we are the sink;
        Q_ASSERT_X(channel.process, "QProcess::start", "Internal error");
        Channel *source = &channel.process->stdoutChannel;
        Channel *sink = &channel;

        if (sink->pipe[0] != INVALID_Q_PIPE) {
            // already constructed by the source
            return true;
        }
        Q_ASSERT(sink == &stdinChannel);
        Q_ASSERT(source->process == this && source->type == Channel::PipeSource);

        if (!qt_create_pipe(sink->pipe, /* in = */ true, TRUE))  // sink is stdin
            return false;

        source->pipe[1] = sink->pipe[1];
        sink->pipe[1] = INVALID_Q_PIPE;

        return true;
    }
    } // switch (channel.type)
    return false;
}

void QProcessPrivate::destroyPipe(Q_PIPE pipe[2])
{
    if (pipe[0] != INVALID_Q_PIPE) {
        CloseHandle(pipe[0]);
        pipe[0] = INVALID_Q_PIPE;
    }
    if (pipe[1] != INVALID_Q_PIPE) {
        CloseHandle(pipe[1]);
        pipe[1] = INVALID_Q_PIPE;
    }
}

template <class T>
void deleteWorker(T *&worker)
{
    if (!worker)
        return;
    worker->stop();
    worker->deleteLater();
    worker = nullptr;
}

void QProcessPrivate::closeChannel(Channel *channel)
{
    if (channel == &stdinChannel)
        deleteWorker(channel->writer);
    else
        deleteWorker(channel->reader);
    destroyPipe(channel->pipe);
}

void QProcessPrivate::cleanup()
{
    q_func()->setProcessState(QProcess::NotRunning);

    closeChannels();
    delete stdinWriteTrigger;
    stdinWriteTrigger = nullptr;
    delete processFinishedNotifier;
    processFinishedNotifier = nullptr;
    if (pid) {
        CloseHandle(pid->hThread);
        CloseHandle(pid->hProcess);
        delete pid;
        pid = nullptr;
    }
}

static QString qt_create_commandline(const QString &program, const QStringList &arguments,
                                     const QString &nativeArguments)
{
    QString args;
    if (!program.isEmpty()) {
        QString programName = program;
        if (!programName.startsWith(QLatin1Char('\"')) && !programName.endsWith(QLatin1Char('\"')) && programName.contains(QLatin1Char(' ')))
            programName = QLatin1Char('\"') + programName + QLatin1Char('\"');
        programName.replace(QLatin1Char('/'), QLatin1Char('\\'));

        // add the program as the first arg ... it works better
        args = programName + QLatin1Char(' ');
    }

    for (qsizetype i = 0; i < arguments.size(); ++i) {
        QString tmp = arguments.at(i);
        // Quotes are escaped and their preceding backslashes are doubled.
        qsizetype index = tmp.indexOf(QLatin1Char('"'));
        while (index >= 0) {
            // Escape quote
            tmp.insert(index++, QLatin1Char('\\'));
            // Double preceding backslashes (ignoring the one we just inserted)
            for (qsizetype i = index - 2 ; i >= 0 && tmp.at(i) == QLatin1Char('\\') ; --i) {
                tmp.insert(i, QLatin1Char('\\'));
                index++;
            }
            index = tmp.indexOf(QLatin1Char('"'), index + 1);
        }
        if (tmp.isEmpty() || tmp.contains(QLatin1Char(' ')) || tmp.contains(QLatin1Char('\t'))) {
            // The argument must not end with a \ since this would be interpreted
            // as escaping the quote -- rather put the \ behind the quote: e.g.
            // rather use "foo"\ than "foo\"
            qsizetype i = tmp.length();
            while (i > 0 && tmp.at(i - 1) == QLatin1Char('\\'))
                --i;
            tmp.insert(i, QLatin1Char('"'));
            tmp.prepend(QLatin1Char('"'));
        }
        args += QLatin1Char(' ') + tmp;
    }

    if (!nativeArguments.isEmpty()) {
        if (!args.isEmpty())
             args += QLatin1Char(' ');
        args += nativeArguments;
    }

    return args;
}

static QByteArray qt_create_environment(const QProcessEnvironmentPrivate::Map &environment)
{
    QByteArray envlist;
    if (!environment.isEmpty()) {
        QProcessEnvironmentPrivate::Map copy = environment;

        // add PATH if necessary (for DLL loading)
        QProcessEnvironmentPrivate::Key pathKey(QLatin1String("PATH"));
        if (!copy.contains(pathKey)) {
            QByteArray path = qgetenv("PATH");
            if (!path.isEmpty())
                copy.insert(pathKey, QString::fromLocal8Bit(path));
        }

        // add systemroot if needed
        QProcessEnvironmentPrivate::Key rootKey(QLatin1String("SystemRoot"));
        if (!copy.contains(rootKey)) {
            QByteArray systemRoot = qgetenv("SystemRoot");
            if (!systemRoot.isEmpty())
                copy.insert(rootKey, QString::fromLocal8Bit(systemRoot));
        }

        qsizetype pos = 0;
        auto it = copy.constBegin();
        const auto end = copy.constEnd();

        static const wchar_t equal = L'=';
        static const wchar_t nul = L'\0';

        for ( ; it != end; ++it) {
            qsizetype tmpSize = sizeof(wchar_t) * (it.key().length() + it.value().length() + 2);
            // ignore empty strings
            if (tmpSize == sizeof(wchar_t) * 2)
                continue;
            envlist.resize(envlist.size() + tmpSize);

            tmpSize = it.key().length() * sizeof(wchar_t);
            memcpy(envlist.data()+pos, it.key().utf16(), tmpSize);
            pos += tmpSize;

            memcpy(envlist.data()+pos, &equal, sizeof(wchar_t));
            pos += sizeof(wchar_t);

            tmpSize = it.value().length() * sizeof(wchar_t);
            memcpy(envlist.data()+pos, it.value().utf16(), tmpSize);
            pos += tmpSize;

            memcpy(envlist.data()+pos, &nul, sizeof(wchar_t));
            pos += sizeof(wchar_t);
        }
        // add the 2 terminating 0 (actually 4, just to be on the safe side)
        envlist.resize( envlist.size()+4 );
        envlist[pos++] = 0;
        envlist[pos++] = 0;
        envlist[pos++] = 0;
        envlist[pos++] = 0;
    }
    return envlist;
}

static Q_PIPE pipeOrStdHandle(Q_PIPE pipe, DWORD handleNumber)
{
    return pipe != INVALID_Q_PIPE ? pipe : GetStdHandle(handleNumber);
}

STARTUPINFOW QProcessPrivate::createStartupInfo()
{
    Q_PIPE stdinPipe = pipeOrStdHandle(stdinChannel.pipe[0], STD_INPUT_HANDLE);
    Q_PIPE stdoutPipe = pipeOrStdHandle(stdoutChannel.pipe[1], STD_OUTPUT_HANDLE);
    Q_PIPE stderrPipe = stderrChannel.pipe[1];
    if (stderrPipe == INVALID_Q_PIPE) {
        stderrPipe = (processChannelMode == QProcess::MergedChannels)
                     ? stdoutPipe
                     : GetStdHandle(STD_ERROR_HANDLE);
    }

    return STARTUPINFOW{
        sizeof(STARTUPINFOW), 0, 0, 0,
        (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
        (ulong)CW_USEDEFAULT, (ulong)CW_USEDEFAULT,
        0, 0, 0,
        STARTF_USESTDHANDLES,
        0, 0, 0,
        stdinPipe, stdoutPipe, stderrPipe
    };
}

bool QProcessPrivate::callCreateProcess(QProcess::CreateProcessArguments *cpargs)
{
    if (modifyCreateProcessArgs)
        modifyCreateProcessArgs(cpargs);
    bool success = CreateProcess(cpargs->applicationName, cpargs->arguments,
                                 cpargs->processAttributes, cpargs->threadAttributes,
                                 cpargs->inheritHandles, cpargs->flags, cpargs->environment,
                                 cpargs->currentDirectory, cpargs->startupInfo,
                                 cpargs->processInformation);
    if (stdinChannel.pipe[0] != INVALID_Q_PIPE) {
        CloseHandle(stdinChannel.pipe[0]);
        stdinChannel.pipe[0] = INVALID_Q_PIPE;
    }
    if (stdoutChannel.pipe[1] != INVALID_Q_PIPE) {
        CloseHandle(stdoutChannel.pipe[1]);
        stdoutChannel.pipe[1] = INVALID_Q_PIPE;
    }
    if (stderrChannel.pipe[1] != INVALID_Q_PIPE) {
        CloseHandle(stderrChannel.pipe[1]);
        stderrChannel.pipe[1] = INVALID_Q_PIPE;
    }
    return success;
}

void QProcessPrivate::startProcess()
{
    Q_Q(QProcess);

    bool success = false;

    if (pid) {
        CloseHandle(pid->hThread);
        CloseHandle(pid->hProcess);
        delete pid;
        pid = 0;
    }
    pid = new PROCESS_INFORMATION;
    memset(pid, 0, sizeof(PROCESS_INFORMATION));

    q->setProcessState(QProcess::Starting);

    if (!openChannels()) {
        QString errorString = QProcess::tr("Process failed to start: %1").arg(qt_error_string());
        cleanup();
        setErrorAndEmit(QProcess::FailedToStart, errorString);
        q->setProcessState(QProcess::NotRunning);
        return;
    }

    const QString args = qt_create_commandline(program, arguments, nativeArguments);
    QByteArray envlist;
    if (environment.d.constData())
        envlist = qt_create_environment(environment.d.constData()->vars);

#if defined QPROCESS_DEBUG
    qDebug("Creating process");
    qDebug("   program : [%s]", program.toLatin1().constData());
    qDebug("   args : %s", args.toLatin1().constData());
    qDebug("   pass environment : %s", environment.isEmpty() ? "no" : "yes");
#endif

    // We cannot unconditionally set the CREATE_NO_WINDOW flag, because this
    // will render the stdout/stderr handles connected to a console useless
    // (this typically affects ForwardedChannels mode).
    // However, we also do not want console tools launched from a GUI app to
    // create new console windows (behavior consistent with UNIX).
    DWORD dwCreationFlags = (GetConsoleWindow() ? 0 : CREATE_NO_WINDOW);
    dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
    STARTUPINFOW startupInfo = createStartupInfo();
    const QString nativeWorkingDirectory = QDir::toNativeSeparators(workingDirectory);
    QProcess::CreateProcessArguments cpargs = {
        nullptr, reinterpret_cast<wchar_t *>(const_cast<ushort *>(args.utf16())),
        nullptr, nullptr, true, dwCreationFlags,
        environment.isEmpty() ? nullptr : envlist.data(),
        nativeWorkingDirectory.isEmpty()
            ? nullptr : reinterpret_cast<const wchar_t *>(nativeWorkingDirectory.utf16()),
        &startupInfo, pid
    };
    success = callCreateProcess(&cpargs);

    QString errorString;
    if (!success) {
        // Capture the error string before we do CloseHandle below
        errorString = QProcess::tr("Process failed to start: %1").arg(qt_error_string());
    }

    if (!success) {
        cleanup();
        setErrorAndEmit(QProcess::FailedToStart, errorString);
        q->setProcessState(QProcess::NotRunning);
        return;
    }

    q->setProcessState(QProcess::Running);
    // User can call kill()/terminate() from the stateChanged() slot
    // so check before proceeding
    if (!pid)
        return;

    if (threadData.loadRelaxed()->hasEventDispatcher()) {
        processFinishedNotifier = new QWinEventNotifier(pid->hProcess, q);
        QObject::connect(processFinishedNotifier, SIGNAL(activated(HANDLE)), q, SLOT(_q_processDied()));
        processFinishedNotifier->setEnabled(true);
    }

    _q_startupNotification();
}

bool QProcessPrivate::processStarted(QString * /*errorMessage*/)
{
    return processState == QProcess::Running;
}

qint64 QProcessPrivate::bytesAvailableInChannel(const Channel *channel) const
{
    Q_ASSERT(channel->pipe[0] != INVALID_Q_PIPE);
    Q_ASSERT(channel->reader);

    DWORD bytesAvail = channel->reader->bytesAvailable();
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::bytesAvailableInChannel(%d) == %lld",
           int(channel - &stdinChannel), qint64(bytesAvail));
#endif
    return bytesAvail;
}

qint64 QProcessPrivate::readFromChannel(const Channel *channel, char *data, qint64 maxlen)
{
    Q_ASSERT(channel->pipe[0] != INVALID_Q_PIPE);
    Q_ASSERT(channel->reader);
    return channel->reader->read(data, maxlen);
}

static BOOL QT_WIN_CALLBACK qt_terminateApp(HWND hwnd, LPARAM procId)
{
    DWORD currentProcId = 0;
    GetWindowThreadProcessId(hwnd, &currentProcId);
    if (currentProcId == (DWORD)procId)
        PostMessage(hwnd, WM_CLOSE, 0, 0);

    return TRUE;
}

void QProcessPrivate::terminateProcess()
{
    if (pid) {
        EnumWindows(qt_terminateApp, (LPARAM)pid->dwProcessId);
        PostThreadMessage(pid->dwThreadId, WM_CLOSE, 0, 0);
    }
}

void QProcessPrivate::killProcess()
{
    if (pid)
        TerminateProcess(pid->hProcess, 0xf291);
}

bool QProcessPrivate::waitForStarted(const QDeadlineTimer &)
{
    if (processStarted())
        return true;

    if (processError == QProcess::FailedToStart)
        return false;

    setError(QProcess::Timedout);
    return false;
}

bool QProcessPrivate::drainOutputPipes()
{
    bool readyReadEmitted = false;

    if (stdoutChannel.reader) {
        stdoutChannel.reader->drainAndStop();
        readyReadEmitted = _q_canReadStandardOutput();
    }
    if (stderrChannel.reader) {
        stderrChannel.reader->drainAndStop();
        readyReadEmitted |= _q_canReadStandardError();
    }

    return readyReadEmitted;
}

bool QProcessPrivate::waitForReadyRead(const QDeadlineTimer &deadline)
{
    forever {
        if (!writeBuffer.isEmpty() && !_q_canWrite())
            return false;

        QProcessPoller poller(*this);
        int ret = poller.poll(deadline);
        if (ret < 0)
            return false;
        if (ret == 0)
            break;

        if (stdinChannel.writer)
            stdinChannel.writer->checkForWrite();

        if ((stdoutChannel.reader && stdoutChannel.reader->checkForReadyRead())
            || (stderrChannel.reader && stderrChannel.reader->checkForReadyRead()))
            return true;

        if (!pid)
            return false;

        if (WaitForSingleObject(pid->hProcess, 0) == WAIT_OBJECT_0) {
            bool readyReadEmitted = drainOutputPipes();
            if (pid)
                processFinished();
            return readyReadEmitted;
        }
    }

    setError(QProcess::Timedout);
    return false;
}

bool QProcessPrivate::waitForBytesWritten(const QDeadlineTimer &deadline)
{
    forever {
        // If no write is pending, try to start one. However, at entry into
        // the loop the write buffer can be empty to start with, in which
        // case _q_caWrite() fails immediately.
        if (pipeWriterBytesToWrite() == 0 && !_q_canWrite())
            return false;

        QProcessPoller poller(*this);
        int ret = poller.poll(deadline);
        if (ret < 0)
            return false;
        if (ret == 0)
            break;

        Q_ASSERT(stdinChannel.writer);
        if (stdinChannel.writer->checkForWrite())
            return true;

        // If we wouldn't write anything, check if we can read stdout.
        if (stdoutChannel.reader)
            stdoutChannel.reader->checkForReadyRead();

        // Check if we can read stderr.
        if (stderrChannel.reader)
            stderrChannel.reader->checkForReadyRead();

        // Check if the process died while reading.
        if (!pid)
            return false;

        // Check if the process is signaling completion.
        if (WaitForSingleObject(pid->hProcess, 0) == WAIT_OBJECT_0) {
            drainOutputPipes();
            if (pid)
                processFinished();
            return false;
        }
    }

    setError(QProcess::Timedout);
    return false;
}

bool QProcessPrivate::waitForFinished(const QDeadlineTimer &deadline)
{
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::waitForFinished(%lld)", deadline.remainingTime());
#endif

    forever {
        if (!writeBuffer.isEmpty() && !_q_canWrite())
            return false;

        QProcessPoller poller(*this);
        int ret = poller.poll(deadline);
        if (ret < 0)
            return false;
        if (ret == 0)
            break;

        if (stdinChannel.writer)
            stdinChannel.writer->checkForWrite();
        if (stdoutChannel.reader)
            stdoutChannel.reader->checkForReadyRead();
        if (stderrChannel.reader)
            stderrChannel.reader->checkForReadyRead();

        if (!pid)
            return true;

        if (WaitForSingleObject(pid->hProcess, 0) == WAIT_OBJECT_0) {
            drainOutputPipes();
            if (pid)
                processFinished();
            return true;
        }
    }

    setError(QProcess::Timedout);
    return false;
}

void QProcessPrivate::findExitCode()
{
    DWORD theExitCode;
    Q_ASSERT(pid);
    if (GetExitCodeProcess(pid->hProcess, &theExitCode)) {
        exitCode = theExitCode;
        crashed = (exitCode == 0xf291   // our magic number, see killProcess
                   || (theExitCode >= 0x80000000 && theExitCode < 0xD0000000));
    }
}

/*! \reimp
    \internal
*/
qint64 QProcess::writeData(const char *data, qint64 len)
{
    Q_D(QProcess);

    if (d->stdinChannel.closed) {
#if defined QPROCESS_DEBUG
        qDebug("QProcess::writeData(%p \"%s\", %lld) == 0 (write channel closing)",
               data, QtDebugUtils::toPrintable(data, len, 16).constData(), len);
#endif
        return 0;
    }

    if (!d->stdinWriteTrigger) {
        d->stdinWriteTrigger = new QTimer;
        d->stdinWriteTrigger->setSingleShot(true);
        QObjectPrivate::connect(d->stdinWriteTrigger, &QTimer::timeout,
                                d, &QProcessPrivate::_q_canWrite);
    }

    d->write(data, len);
    if (!d->stdinWriteTrigger->isActive())
        d->stdinWriteTrigger->start();

#if defined QPROCESS_DEBUG
    qDebug("QProcess::writeData(%p \"%s\", %lld) == %lld (written to buffer)",
           data, QtDebugUtils::toPrintable(data, len, 16).constData(), len, len);
#endif
    return len;
}

qint64 QProcessPrivate::pipeWriterBytesToWrite() const
{
    return stdinChannel.writer ? stdinChannel.writer->bytesToWrite() : qint64(0);
}

void QProcessPrivate::_q_bytesWritten(qint64 bytes)
{
    Q_Q(QProcess);

    if (!emittedBytesWritten) {
        QScopedValueRollback<bool> guard(emittedBytesWritten, true);
        emit q->bytesWritten(bytes);
    }
    _q_canWrite();
}

bool QProcessPrivate::_q_canWrite()
{
    if (writeBuffer.isEmpty()) {
        if (stdinChannel.closed && pipeWriterBytesToWrite() == 0)
            closeWriteChannel();
#if defined QPROCESS_DEBUG
        qDebug("QProcessPrivate::canWrite(), not writing anything (empty write buffer).");
#endif
        return false;
    }

    return writeToStdin();
}

bool QProcessPrivate::writeToStdin()
{
    Q_Q(QProcess);

    if (!stdinChannel.writer) {
        stdinChannel.writer = new QWindowsPipeWriter(stdinChannel.pipe[1], q);
        QObjectPrivate::connect(stdinChannel.writer, &QWindowsPipeWriter::bytesWritten,
                                this, &QProcessPrivate::_q_bytesWritten);
    } else {
        if (stdinChannel.writer->isWriteOperationActive())
            return true;
    }

    stdinChannel.writer->write(writeBuffer.read());
    return true;
}

// Use ShellExecuteEx() to trigger an UAC prompt when CreateProcess()fails
// with ERROR_ELEVATION_REQUIRED.
static bool startDetachedUacPrompt(const QString &programIn, const QStringList &arguments,
                                   const QString &nativeArguments,
                                   const QString &workingDir, qint64 *pid)
{
    const QString args = qt_create_commandline(QString(),                   // needs arguments only
                                               arguments, nativeArguments);
    SHELLEXECUTEINFOW shellExecuteExInfo;
    memset(&shellExecuteExInfo, 0, sizeof(SHELLEXECUTEINFOW));
    shellExecuteExInfo.cbSize = sizeof(SHELLEXECUTEINFOW);
    shellExecuteExInfo.fMask = SEE_MASK_NOCLOSEPROCESS | SEE_MASK_UNICODE | SEE_MASK_FLAG_NO_UI | SEE_MASK_CLASSNAME;
    shellExecuteExInfo.lpClass = L"exefile";
    shellExecuteExInfo.lpVerb = L"runas";
    const QString program = QDir::toNativeSeparators(programIn);
    shellExecuteExInfo.lpFile = reinterpret_cast<LPCWSTR>(program.utf16());
    if (!args.isEmpty())
        shellExecuteExInfo.lpParameters = reinterpret_cast<LPCWSTR>(args.utf16());
    if (!workingDir.isEmpty())
        shellExecuteExInfo.lpDirectory = reinterpret_cast<LPCWSTR>(workingDir.utf16());
    shellExecuteExInfo.nShow = SW_SHOWNORMAL;

    if (!ShellExecuteExW(&shellExecuteExInfo))
        return false;
    if (pid)
        *pid = qint64(GetProcessId(shellExecuteExInfo.hProcess));
    CloseHandle(shellExecuteExInfo.hProcess);
    return true;
}

bool QProcessPrivate::startDetached(qint64 *pid)
{
    static const DWORD errorElevationRequired = 740;

    if (!openChannelsForDetached()) {
        // openChannel sets the error string
        closeChannels();
        return false;
    }

    QString args = qt_create_commandline(program, arguments, nativeArguments);
    bool success = false;
    PROCESS_INFORMATION pinfo;

    void *envPtr = nullptr;
    QByteArray envlist;
    if (environment.d.constData()) {
        envlist = qt_create_environment(environment.d.constData()->vars);
        envPtr = envlist.data();
    }

    DWORD dwCreationFlags = (GetConsoleWindow() ? 0 : CREATE_NO_WINDOW);
    dwCreationFlags |= CREATE_UNICODE_ENVIRONMENT;
    STARTUPINFOW startupInfo = createStartupInfo();
    QProcess::CreateProcessArguments cpargs = {
        nullptr, reinterpret_cast<wchar_t *>(const_cast<ushort *>(args.utf16())),
        nullptr, nullptr, true, dwCreationFlags, envPtr,
        workingDirectory.isEmpty()
            ? nullptr : reinterpret_cast<const wchar_t *>(workingDirectory.utf16()),
        &startupInfo, &pinfo
    };
    success = callCreateProcess(&cpargs);

    if (success) {
        CloseHandle(pinfo.hThread);
        CloseHandle(pinfo.hProcess);
        if (pid)
            *pid = pinfo.dwProcessId;
    } else if (GetLastError() == errorElevationRequired) {
        if (envPtr)
            qWarning("QProcess: custom environment will be ignored for detached elevated process.");
        if (!stdinChannel.file.isEmpty() || !stdoutChannel.file.isEmpty()
                || !stderrChannel.file.isEmpty()) {
            qWarning("QProcess: file redirection is unsupported for detached elevated processes.");
        }
        success = startDetachedUacPrompt(program, arguments, nativeArguments,
                                         workingDirectory, pid);
    }
    if (!success) {
        if (pid)
            *pid = -1;
        setErrorAndEmit(QProcess::FailedToStart);
    }

    closeChannels();
    return success;
}

#endif // QT_CONFIG(process)

QT_END_NAMESPACE
