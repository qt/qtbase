/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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
#include "qdebug.h"

#ifndef QT_NO_PROCESS

#if defined QPROCESS_DEBUG
#include "private/qtools_p.h"
#include <ctype.h>

/*
    Returns a human readable representation of the first \a len
    characters in \a data.
*/
QT_BEGIN_NAMESPACE
static QByteArray qt_prettyDebug(const char *data, int len, int maxSize)
{
    if (!data) return "(null)";
    QByteArray out;
    for (int i = 0; i < len; ++i) {
        char c = data[i];
        if (isprint(c)) {
            out += c;
        } else switch (c) {
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default: {
            const char buf[] =  {
                '\\',
                QtMiscUtils::toOct(uchar(c) / 64),
                QtMiscUtils::toOct(uchar(c) % 64 / 8),
                QtMiscUtils::toOct(uchar(c) % 8),
                0
            };
            out += buf;
            }
        }
    }

    if (len < maxSize)
        out += "...";

    return out;
}
QT_END_NAMESPACE
#endif

#include "qplatformdefs.h"

#include "qprocess.h"
#include "qprocess_p.h"
#include "private/qcore_unix_p.h"

#ifdef Q_OS_MAC
#include <private/qcore_mac_p.h>
#endif

#include <private/qcoreapplication_p.h>
#include <private/qthread_p.h>
#include <qfile.h>
#include <qfileinfo.h>
#include <qdir.h>
#include <qlist.h>
#include <qmutex.h>
#include <qsemaphore.h>
#include <qsocketnotifier.h>
#include <qthread.h>
#include <qelapsedtimer.h>

#ifdef Q_OS_QNX
#  include <sys/neutrino.h>
#endif

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <forkfd.h>

QT_BEGIN_NAMESPACE

// POSIX requires PIPE_BUF to be 512 or larger
// so we will use 512
static const int errorBufferMax = 512;

namespace {
struct QProcessPoller
{
    QProcessPoller(const QProcessPrivate &proc);

    int poll(int timeout);

    pollfd &stdinPipe() { return pfds[0]; }
    pollfd &stdoutPipe() { return pfds[1]; }
    pollfd &stderrPipe() { return pfds[2]; }
    pollfd &forkfd() { return pfds[3]; }
    pollfd &childStartedPipe() { return pfds[4]; }

    enum { n_pfds = 5 };
    pollfd pfds[n_pfds];
};

QProcessPoller::QProcessPoller(const QProcessPrivate &proc)
{
    for (int i = 0; i < n_pfds; i++)
        pfds[i] = qt_make_pollfd(-1, POLLIN);

    stdoutPipe().fd = proc.stdoutChannel.pipe[0];
    stderrPipe().fd = proc.stderrChannel.pipe[0];

    if (!proc.writeBuffer.isEmpty()) {
        stdinPipe().fd = proc.stdinChannel.pipe[1];
        stdinPipe().events = POLLOUT;
    }

    forkfd().fd = proc.forkfd;

    if (proc.processState == QProcess::Starting)
        childStartedPipe().fd = proc.childStartedPipe[0];
}

int QProcessPoller::poll(int timeout)
{
    const nfds_t nfds = (childStartedPipe().fd == -1) ? 4 : 5;
    return qt_poll_msecs(pfds, nfds, timeout);
}
} // anonymous namespace

static bool qt_pollfd_check(const pollfd &pfd, short revents)
{
    return pfd.fd >= 0 && (pfd.revents & (revents | POLLHUP | POLLERR | POLLNVAL)) != 0;
}

static int qt_create_pipe(int *pipe)
{
    if (pipe[0] != -1)
        qt_safe_close(pipe[0]);
    if (pipe[1] != -1)
        qt_safe_close(pipe[1]);
    int pipe_ret = qt_safe_pipe(pipe);
    if (pipe_ret != 0) {
        qWarning("QProcessPrivate::createPipe: Cannot create pipe %p: %s",
                 pipe, qPrintable(qt_error_string(errno)));
    }
    return pipe_ret;
}

void QProcessPrivate::destroyPipe(int *pipe)
{
    if (pipe[1] != -1) {
        qt_safe_close(pipe[1]);
        pipe[1] = -1;
    }
    if (pipe[0] != -1) {
        qt_safe_close(pipe[0]);
        pipe[0] = -1;
    }
}

void QProcessPrivate::closeChannel(Channel *channel)
{
    destroyPipe(channel->pipe);
}

/*
    Create the pipes to a QProcessPrivate::Channel.

    This function must be called in order: stdin, stdout, stderr
*/
bool QProcessPrivate::openChannel(Channel &channel)
{
    Q_Q(QProcess);

    if (&channel == &stderrChannel && processChannelMode == QProcess::MergedChannels) {
        channel.pipe[0] = -1;
        channel.pipe[1] = -1;
        return true;
    }

    if (channel.type == Channel::Normal) {
        // we're piping this channel to our own process
        if (qt_create_pipe(channel.pipe) != 0)
            return false;

        // create the socket notifiers
        if (threadData->hasEventDispatcher()) {
            if (&channel == &stdinChannel) {
                channel.notifier = new QSocketNotifier(channel.pipe[1],
                                                       QSocketNotifier::Write, q);
                channel.notifier->setEnabled(false);
                QObject::connect(channel.notifier, SIGNAL(activated(int)),
                                 q, SLOT(_q_canWrite()));
            } else {
                channel.notifier = new QSocketNotifier(channel.pipe[0],
                                                       QSocketNotifier::Read, q);
                const char *receiver;
                if (&channel == &stdoutChannel)
                    receiver = SLOT(_q_canReadStandardOutput());
                else
                    receiver = SLOT(_q_canReadStandardError());
                QObject::connect(channel.notifier, SIGNAL(activated(int)),
                                 q, receiver);
            }
        }

        return true;
    } else if (channel.type == Channel::Redirect) {
        // we're redirecting the channel to/from a file
        QByteArray fname = QFile::encodeName(channel.file);

        if (&channel == &stdinChannel) {
            // try to open in read-only mode
            channel.pipe[1] = -1;
            if ( (channel.pipe[0] = qt_safe_open(fname, O_RDONLY)) != -1)
                return true;    // success
            setErrorAndEmit(QProcess::FailedToStart,
                            QProcess::tr("Could not open input redirection for reading"));
        } else {
            int mode = O_WRONLY | O_CREAT;
            if (channel.append)
                mode |= O_APPEND;
            else
                mode |= O_TRUNC;

            channel.pipe[0] = -1;
            if ( (channel.pipe[1] = qt_safe_open(fname, mode, 0666)) != -1)
                return true; // success

            setErrorAndEmit(QProcess::FailedToStart,
                            QProcess::tr("Could not open input redirection for reading"));
        }
        cleanup();
        return false;
    } else {
        Q_ASSERT_X(channel.process, "QProcess::start", "Internal error");

        Channel *source;
        Channel *sink;

        if (channel.type == Channel::PipeSource) {
            // we are the source
            source = &channel;
            sink = &channel.process->stdinChannel;

            Q_ASSERT(source == &stdoutChannel);
            Q_ASSERT(sink->process == this && sink->type == Channel::PipeSink);
        } else {
            // we are the sink;
            source = &channel.process->stdoutChannel;
            sink = &channel;

            Q_ASSERT(sink == &stdinChannel);
            Q_ASSERT(source->process == this && source->type == Channel::PipeSource);
        }

        if (source->pipe[1] != INVALID_Q_PIPE || sink->pipe[0] != INVALID_Q_PIPE) {
            // already created, do nothing
            return true;
        } else {
            Q_ASSERT(source->pipe[0] == INVALID_Q_PIPE && source->pipe[1] == INVALID_Q_PIPE);
            Q_ASSERT(sink->pipe[0] == INVALID_Q_PIPE && sink->pipe[1] == INVALID_Q_PIPE);

            Q_PIPE pipe[2] = { -1, -1 };
            if (qt_create_pipe(pipe) != 0)
                return false;
            sink->pipe[0] = pipe[0];
            source->pipe[1] = pipe[1];

            return true;
        }
    }
}

QT_BEGIN_INCLUDE_NAMESPACE
#if defined(Q_OS_MACX)
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
#else
  extern char **environ;
#endif
QT_END_INCLUDE_NAMESPACE

QProcessEnvironment QProcessEnvironment::systemEnvironment()
{
    QProcessEnvironment env;
#if !defined(QT_PLATFORM_UIKIT)
    const char *entry;
    for (int count = 0; (entry = environ[count]); ++count) {
        const char *equal = strchr(entry, '=');
        if (!equal)
            continue;

        QByteArray name(entry, equal - entry);
        QByteArray value(equal + 1);
        env.d->hash.insert(QProcessEnvironmentPrivate::Key(name),
                           QProcessEnvironmentPrivate::Value(value));
    }
#endif
    return env;
}

static char **_q_dupEnvironment(const QProcessEnvironmentPrivate::Hash &environment, int *envc)
{
    *envc = 0;
    if (environment.isEmpty())
        return 0;

    char **envp = new char *[environment.count() + 2];
    envp[environment.count()] = 0;
    envp[environment.count() + 1] = 0;

    QProcessEnvironmentPrivate::Hash::ConstIterator it = environment.constBegin();
    const QProcessEnvironmentPrivate::Hash::ConstIterator end = environment.constEnd();
    for ( ; it != end; ++it) {
        QByteArray key = it.key().key;
        QByteArray value = it.value().bytes();
        key.reserve(key.length() + 1 + value.length());
        key.append('=');
        key.append(value);

        envp[(*envc)++] = ::strdup(key.constData());
    }

    return envp;
}

void QProcessPrivate::startProcess()
{
    Q_Q(QProcess);

#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::startProcess()");
#endif

    // Initialize pipes
    if (!openChannel(stdinChannel) ||
        !openChannel(stdoutChannel) ||
        !openChannel(stderrChannel) ||
        qt_create_pipe(childStartedPipe) != 0) {
        setErrorAndEmit(QProcess::FailedToStart, qt_error_string(errno));
        cleanup();
        return;
    }

    if (threadData->hasEventDispatcher()) {
        startupSocketNotifier = new QSocketNotifier(childStartedPipe[0],
                                                    QSocketNotifier::Read, q);
        QObject::connect(startupSocketNotifier, SIGNAL(activated(int)),
                         q, SLOT(_q_startupNotification()));
    }

    // Start the process (platform dependent)
    q->setProcessState(QProcess::Starting);

    // Create argument list with right number of elements, and set the final
    // one to 0.
    char **argv = new char *[arguments.count() + 2];
    argv[arguments.count() + 1] = 0;

    // Encode the program name.
    QByteArray encodedProgramName = QFile::encodeName(program);
#ifdef Q_OS_MAC
    // allow invoking of .app bundles on the Mac.
    QFileInfo fileInfo(program);
    if (encodedProgramName.endsWith(".app") && fileInfo.isDir()) {
        QCFType<CFURLRef> url = CFURLCreateWithFileSystemPath(0,
                                                          QCFString(fileInfo.absoluteFilePath()),
                                                          kCFURLPOSIXPathStyle, true);
        {
            // CFBundle is not reentrant, since CFBundleCreate might return a reference
            // to a cached bundle object. Protect the bundle calls with a mutex lock.
            static QBasicMutex cfbundleMutex;
            QMutexLocker lock(&cfbundleMutex);
            QCFType<CFBundleRef> bundle = CFBundleCreate(0, url);
            // 'executableURL' can be either relative or absolute ...
            QCFType<CFURLRef> executableURL = CFBundleCopyExecutableURL(bundle);
            // not to depend on caching - make sure it's always absolute.
            url = CFURLCopyAbsoluteURL(executableURL);
        }
        if (url) {
            const QCFString str = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
            encodedProgramName += (QDir::separator() + QDir(program).relativeFilePath(QString::fromCFString(str))).toUtf8();
        }
    }
#endif

    // Add the program name to the argument list.
    char *dupProgramName = ::strdup(encodedProgramName.constData());
    argv[0] = dupProgramName;

    // Add every argument to the list
    for (int i = 0; i < arguments.count(); ++i)
        argv[i + 1] = ::strdup(QFile::encodeName(arguments.at(i)).constData());

    // Duplicate the environment.
    int envc = 0;
    char **envp = 0;
    if (environment.d.constData()) {
        QProcessEnvironmentPrivate::MutexLocker locker(environment.d);
        envp = _q_dupEnvironment(environment.d.constData()->hash, &envc);
    }

    // Encode the working directory if it's non-empty, otherwise just pass 0.
    const char *workingDirPtr = 0;
    QByteArray encodedWorkingDirectory;
    if (!workingDirectory.isEmpty()) {
        encodedWorkingDirectory = QFile::encodeName(workingDirectory);
        workingDirPtr = encodedWorkingDirectory.constData();
    }

    // If the program does not specify a path, generate a list of possible
    // locations for the binary using the PATH environment variable.
    char **path = 0;
    int pathc = 0;
    if (!program.contains(QLatin1Char('/'))) {
        const QString pathEnv = QString::fromLocal8Bit(qgetenv("PATH"));
        if (!pathEnv.isEmpty()) {
            QStringList pathEntries = pathEnv.split(QLatin1Char(':'), QString::SkipEmptyParts);
            if (!pathEntries.isEmpty()) {
                pathc = pathEntries.size();
                path = new char *[pathc + 1];
                path[pathc] = 0;

                for (int k = 0; k < pathEntries.size(); ++k) {
                    QByteArray tmp = QFile::encodeName(pathEntries.at(k));
                    if (!tmp.endsWith('/')) tmp += '/';
                    tmp += encodedProgramName;
                    path[k] = ::strdup(tmp.constData());
                }
            }
        }
    }

    // Start the process manager, and fork off the child process.
    pid_t childPid;
    forkfd = ::forkfd(FFD_CLOEXEC, &childPid);
    int lastForkErrno = errno;
    if (forkfd != FFD_CHILD_PROCESS) {
        // Parent process.
        // Clean up duplicated memory.
        free(dupProgramName);
        for (int i = 1; i <= arguments.count(); ++i)
            free(argv[i]);
        for (int i = 0; i < envc; ++i)
            free(envp[i]);
        for (int i = 0; i < pathc; ++i)
            free(path[i]);
        delete [] argv;
        delete [] envp;
        delete [] path;
    }

    // On QNX, if spawnChild failed, childPid will be -1 but forkfd is still 0.
    // This is intentional because we only want to handle failure to fork()
    // here, which is a rare occurrence. Handling of the failure to start is
    // done elsewhere.
    if (forkfd == -1) {
        // Cleanup, report error and return
#if defined (QPROCESS_DEBUG)
        qDebug("fork failed: %s", qPrintable(qt_error_string(lastForkErrno)));
#endif
        q->setProcessState(QProcess::NotRunning);
        setErrorAndEmit(QProcess::FailedToStart,
                        QProcess::tr("Resource error (fork failure): %1").arg(qt_error_string(lastForkErrno)));
        cleanup();
        return;
    }

    // Start the child.
    if (forkfd == FFD_CHILD_PROCESS) {
        execChild(workingDirPtr, path, argv, envp);
        ::_exit(-1);
    }

    pid = Q_PID(childPid);

    // parent
    // close the ends we don't use and make all pipes non-blocking
    qt_safe_close(childStartedPipe[1]);
    childStartedPipe[1] = -1;

    if (stdinChannel.pipe[0] != -1) {
        qt_safe_close(stdinChannel.pipe[0]);
        stdinChannel.pipe[0] = -1;
    }

    if (stdinChannel.pipe[1] != -1)
        ::fcntl(stdinChannel.pipe[1], F_SETFL, ::fcntl(stdinChannel.pipe[1], F_GETFL) | O_NONBLOCK);

    if (stdoutChannel.pipe[1] != -1) {
        qt_safe_close(stdoutChannel.pipe[1]);
        stdoutChannel.pipe[1] = -1;
    }

    if (stdoutChannel.pipe[0] != -1)
        ::fcntl(stdoutChannel.pipe[0], F_SETFL, ::fcntl(stdoutChannel.pipe[0], F_GETFL) | O_NONBLOCK);

    if (stderrChannel.pipe[1] != -1) {
        qt_safe_close(stderrChannel.pipe[1]);
        stderrChannel.pipe[1] = -1;
    }
    if (stderrChannel.pipe[0] != -1)
        ::fcntl(stderrChannel.pipe[0], F_SETFL, ::fcntl(stderrChannel.pipe[0], F_GETFL) | O_NONBLOCK);

    if (threadData->eventDispatcher) {
        deathNotifier = new QSocketNotifier(forkfd, QSocketNotifier::Read, q);
        QObject::connect(deathNotifier, SIGNAL(activated(int)),
                         q, SLOT(_q_processDied()));
    }
}

void QProcessPrivate::execChild(const char *workingDir, char **path, char **argv, char **envp)
{
    ::signal(SIGPIPE, SIG_DFL);         // reset the signal that we ignored

    Q_Q(QProcess);

    // copy the stdin socket if asked to (without closing on exec)
    if (inputChannelMode != QProcess::ForwardedInputChannel)
        qt_safe_dup2(stdinChannel.pipe[0], STDIN_FILENO, 0);

    // copy the stdout and stderr if asked to
    if (processChannelMode != QProcess::ForwardedChannels) {
        if (processChannelMode != QProcess::ForwardedOutputChannel)
            qt_safe_dup2(stdoutChannel.pipe[1], STDOUT_FILENO, 0);

        // merge stdout and stderr if asked to
        if (processChannelMode == QProcess::MergedChannels) {
            qt_safe_dup2(STDOUT_FILENO, STDERR_FILENO, 0);
        } else if (processChannelMode != QProcess::ForwardedErrorChannel) {
            qt_safe_dup2(stderrChannel.pipe[1], STDERR_FILENO, 0);
        }
    }

    // make sure this fd is closed if execvp() succeeds
    qt_safe_close(childStartedPipe[0]);

    // enter the working directory
    const char *callthatfailed = "chdir: ";
    if (workingDir && QT_CHDIR(workingDir) == -1) {
        // failed, stop the process
        goto report_errno;
    }

    // this is a virtual call, and it base behavior is to do nothing.
    q->setupChildProcess();

    // execute the process
    if (!envp) {
        qt_safe_execvp(argv[0], argv);
        callthatfailed = "execvp: ";
    } else {
        if (path) {
            char **arg = path;
            while (*arg) {
                argv[0] = *arg;
#if defined (QPROCESS_DEBUG)
                fprintf(stderr, "QProcessPrivate::execChild() searching / starting %s\n", argv[0]);
#endif
                qt_safe_execve(argv[0], argv, envp);
                ++arg;
            }
        } else {
#if defined (QPROCESS_DEBUG)
            fprintf(stderr, "QProcessPrivate::execChild() starting %s\n", argv[0]);
#endif
            qt_safe_execve(argv[0], argv, envp);
        }
        callthatfailed = "execve: ";
    }

    // notify failure
    // we're running in the child process, so we don't need to be thread-safe;
    // we can use strerror
report_errno:
    const char *msg = strerror(errno);
#if defined (QPROCESS_DEBUG)
    fprintf(stderr, "QProcessPrivate::execChild() failed (%s), notifying parent process\n", msg);
#endif
    qt_safe_write(childStartedPipe[1], callthatfailed, strlen(callthatfailed));
    qt_safe_write(childStartedPipe[1], msg, strlen(msg));
    qt_safe_close(childStartedPipe[1]);
    childStartedPipe[1] = -1;
}

bool QProcessPrivate::processStarted(QString *errorMessage)
{
    char buf[errorBufferMax];
    int i = 0;
    int ret;
    do {
        ret = qt_safe_read(childStartedPipe[0], buf + i, sizeof buf - i);
        if (ret > 0)
            i += ret;
    } while (ret > 0 && i < int(sizeof buf));

    if (startupSocketNotifier) {
        startupSocketNotifier->setEnabled(false);
        startupSocketNotifier->deleteLater();
        startupSocketNotifier = 0;
    }
    qt_safe_close(childStartedPipe[0]);
    childStartedPipe[0] = -1;

#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::processStarted() == %s", i <= 0 ? "true" : "false");
#endif

    // did we read an error message?
    if ((i > 0) && errorMessage)
        *errorMessage = QString::fromLocal8Bit(buf, i);

    return i <= 0;
}

qint64 QProcessPrivate::bytesAvailableInChannel(const Channel *channel) const
{
    Q_ASSERT(channel->pipe[0] != INVALID_Q_PIPE);
    int nbytes = 0;
    qint64 available = 0;
    if (::ioctl(channel->pipe[0], FIONREAD, (char *) &nbytes) >= 0)
        available = (qint64) nbytes;
#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::bytesAvailableInChannel(%d) == %lld", int(channel - &stdinChannel), available);
#endif
    return available;
}

qint64 QProcessPrivate::readFromChannel(const Channel *channel, char *data, qint64 maxlen)
{
    Q_ASSERT(channel->pipe[0] != INVALID_Q_PIPE);
    qint64 bytesRead = qt_safe_read(channel->pipe[0], data, maxlen);
#if defined QPROCESS_DEBUG
    int save_errno = errno;
    qDebug("QProcessPrivate::readFromChannel(%d, %p \"%s\", %lld) == %lld",
           int(channel - &stdinChannel),
           data, qt_prettyDebug(data, bytesRead, 16).constData(), maxlen, bytesRead);
    errno = save_errno;
#endif
    if (bytesRead == -1 && errno == EWOULDBLOCK)
        return -2;
    return bytesRead;
}

bool QProcessPrivate::writeToStdin()
{
    const char *data = writeBuffer.readPointer();
    const qint64 bytesToWrite = writeBuffer.nextDataBlockSize();

    qint64 written = qt_safe_write_nosignal(stdinChannel.pipe[1], data, bytesToWrite);
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::writeToStdin(), write(%p \"%s\", %lld) == %lld",
           data, qt_prettyDebug(data, bytesToWrite, 16).constData(), bytesToWrite, written);
    if (written == -1)
        qDebug("QProcessPrivate::writeToStdin(), failed to write (%s)", qPrintable(qt_error_string(errno)));
#endif
    if (written == -1) {
        // If the O_NONBLOCK flag is set and If some data can be written without blocking
        // the process, write() will transfer what it can and return the number of bytes written.
        // Otherwise, it will return -1 and set errno to EAGAIN
        if (errno == EAGAIN)
            return true;

        closeChannel(&stdinChannel);
        setErrorAndEmit(QProcess::WriteError);
        return false;
    }
    writeBuffer.free(written);
    if (!emittedBytesWritten && written != 0) {
        emittedBytesWritten = true;
        emit q_func()->bytesWritten(written);
        emittedBytesWritten = false;
    }
    return true;
}

void QProcessPrivate::terminateProcess()
{
#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::terminateProcess()");
#endif
    if (pid)
        ::kill(pid_t(pid), SIGTERM);
}

void QProcessPrivate::killProcess()
{
#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::killProcess()");
#endif
    if (pid)
        ::kill(pid_t(pid), SIGKILL);
}

bool QProcessPrivate::waitForStarted(int msecs)
{
#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::waitForStarted(%d) waiting for child to start (fd = %d)", msecs,
           childStartedPipe[0]);
#endif

    pollfd pfd = qt_make_pollfd(childStartedPipe[0], POLLIN);

    if (qt_poll_msecs(&pfd, 1, msecs) == 0) {
        setError(QProcess::Timedout);
#if defined (QPROCESS_DEBUG)
        qDebug("QProcessPrivate::waitForStarted(%d) == false (timed out)", msecs);
#endif
        return false;
    }

    bool startedEmitted = _q_startupNotification();
#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::waitForStarted() == %s", startedEmitted ? "true" : "false");
#endif
    return startedEmitted;
}

bool QProcessPrivate::waitForReadyRead(int msecs)
{
#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::waitForReadyRead(%d)", msecs);
#endif

    QElapsedTimer stopWatch;
    stopWatch.start();

    forever {
        QProcessPoller poller(*this);

        int timeout = qt_subtract_from_timeout(msecs, stopWatch.elapsed());
        int ret = poller.poll(timeout);

        if (ret < 0) {
            break;
        }
        if (ret == 0) {
            setError(QProcess::Timedout);
            return false;
        }

        if (qt_pollfd_check(poller.childStartedPipe(), POLLIN)) {
            if (!_q_startupNotification())
                return false;
        }

        bool readyReadEmitted = false;
        if (qt_pollfd_check(poller.stdoutPipe(), POLLIN)) {
            bool canRead = _q_canReadStandardOutput();
            if (currentReadChannel == QProcess::StandardOutput && canRead)
                readyReadEmitted = true;
        }
        if (qt_pollfd_check(poller.stderrPipe(), POLLIN)) {
            bool canRead = _q_canReadStandardError();
            if (currentReadChannel == QProcess::StandardError && canRead)
                readyReadEmitted = true;
        }
        if (readyReadEmitted)
            return true;

        if (qt_pollfd_check(poller.stdinPipe(), POLLOUT))
            _q_canWrite();

        if (qt_pollfd_check(poller.forkfd(), POLLIN)) {
            if (_q_processDied())
                return false;
        }
    }
    return false;
}

bool QProcessPrivate::waitForBytesWritten(int msecs)
{
#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::waitForBytesWritten(%d)", msecs);
#endif

    QElapsedTimer stopWatch;
    stopWatch.start();

    while (!writeBuffer.isEmpty()) {
        QProcessPoller poller(*this);

        int timeout = qt_subtract_from_timeout(msecs, stopWatch.elapsed());
        int ret = poller.poll(timeout);

        if (ret < 0) {
            break;
        }

        if (ret == 0) {
            setError(QProcess::Timedout);
            return false;
        }

        if (qt_pollfd_check(poller.childStartedPipe(), POLLIN)) {
            if (!_q_startupNotification())
                return false;
        }

        if (qt_pollfd_check(poller.stdinPipe(), POLLOUT))
            return _q_canWrite();

        if (qt_pollfd_check(poller.stdoutPipe(), POLLIN))
            _q_canReadStandardOutput();

        if (qt_pollfd_check(poller.stderrPipe(), POLLIN))
            _q_canReadStandardError();

        if (qt_pollfd_check(poller.forkfd(), POLLIN)) {
            if (_q_processDied())
                return false;
        }
    }

    return false;
}

bool QProcessPrivate::waitForFinished(int msecs)
{
#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::waitForFinished(%d)", msecs);
#endif

    QElapsedTimer stopWatch;
    stopWatch.start();

    forever {
        QProcessPoller poller(*this);

        int timeout = qt_subtract_from_timeout(msecs, stopWatch.elapsed());
        int ret = poller.poll(timeout);

        if (ret < 0) {
            break;
        }
        if (ret == 0) {
            setError(QProcess::Timedout);
            return false;
        }

        if (qt_pollfd_check(poller.childStartedPipe(), POLLIN)) {
            if (!_q_startupNotification())
                return false;
        }
        if (qt_pollfd_check(poller.stdinPipe(), POLLOUT))
            _q_canWrite();

        if (qt_pollfd_check(poller.stdoutPipe(), POLLIN))
            _q_canReadStandardOutput();

        if (qt_pollfd_check(poller.stderrPipe(), POLLIN))
            _q_canReadStandardError();

        if (qt_pollfd_check(poller.forkfd(), POLLIN)) {
            if (_q_processDied())
                return true;
        }
    }
    return false;
}

void QProcessPrivate::findExitCode()
{
}

bool QProcessPrivate::waitForDeadChild()
{
    if (forkfd == -1)
        return true; // child has already exited

    // read the process information from our fd
    forkfd_info info;
    int ret;
    EINTR_LOOP(ret, forkfd_wait(forkfd, &info, Q_NULLPTR));

    exitCode = info.status;
    crashed = info.code != CLD_EXITED;

    delete deathNotifier;
    deathNotifier = 0;

    EINTR_LOOP(ret, forkfd_close(forkfd));
    forkfd = -1; // Child is dead, don't try to kill it anymore

#if defined QPROCESS_DEBUG
    qDebug() << "QProcessPrivate::waitForDeadChild() dead with exitCode"
             << exitCode << ", crashed?" << crashed;
#endif
    return true;
}

bool QProcessPrivate::startDetached(const QString &program, const QStringList &arguments, const QString &workingDirectory, qint64 *pid)
{
    QByteArray encodedWorkingDirectory = QFile::encodeName(workingDirectory);

    // To catch the startup of the child
    int startedPipe[2];
    if (qt_safe_pipe(startedPipe) != 0)
        return false;
    // To communicate the pid of the child
    int pidPipe[2];
    if (qt_safe_pipe(pidPipe) != 0) {
        qt_safe_close(startedPipe[0]);
        qt_safe_close(startedPipe[1]);
        return false;
    }

    pid_t childPid = fork();
    if (childPid == 0) {
        struct sigaction noaction;
        memset(&noaction, 0, sizeof(noaction));
        noaction.sa_handler = SIG_IGN;
        ::sigaction(SIGPIPE, &noaction, 0);

        ::setsid();

        qt_safe_close(startedPipe[0]);
        qt_safe_close(pidPipe[0]);

        pid_t doubleForkPid = fork();
        if (doubleForkPid == 0) {
            qt_safe_close(pidPipe[1]);

            if (!encodedWorkingDirectory.isEmpty()) {
                if (QT_CHDIR(encodedWorkingDirectory.constData()) == -1)
                    qWarning("QProcessPrivate::startDetached: failed to chdir to %s", encodedWorkingDirectory.constData());
            }

            char **argv = new char *[arguments.size() + 2];
            for (int i = 0; i < arguments.size(); ++i)
                argv[i + 1] = ::strdup(QFile::encodeName(arguments.at(i)).constData());
            argv[arguments.size() + 1] = 0;

            if (!program.contains(QLatin1Char('/'))) {
                const QString path = QString::fromLocal8Bit(qgetenv("PATH"));
                if (!path.isEmpty()) {
                    QStringList pathEntries = path.split(QLatin1Char(':'));
                    for (int k = 0; k < pathEntries.size(); ++k) {
                        QByteArray tmp = QFile::encodeName(pathEntries.at(k));
                        if (!tmp.endsWith('/')) tmp += '/';
                        tmp += QFile::encodeName(program);
                        argv[0] = tmp.data();
                        qt_safe_execv(argv[0], argv);
                    }
                }
            } else {
                QByteArray tmp = QFile::encodeName(program);
                argv[0] = tmp.data();
                qt_safe_execv(argv[0], argv);
            }

            struct sigaction noaction;
            memset(&noaction, 0, sizeof(noaction));
            noaction.sa_handler = SIG_IGN;
            ::sigaction(SIGPIPE, &noaction, 0);

            // '\1' means execv failed
            char c = '\1';
            qt_safe_write(startedPipe[1], &c, 1);
            qt_safe_close(startedPipe[1]);
            ::_exit(1);
        } else if (doubleForkPid == -1) {
            struct sigaction noaction;
            memset(&noaction, 0, sizeof(noaction));
            noaction.sa_handler = SIG_IGN;
            ::sigaction(SIGPIPE, &noaction, 0);

            // '\2' means internal error
            char c = '\2';
            qt_safe_write(startedPipe[1], &c, 1);
        }

        qt_safe_close(startedPipe[1]);
        qt_safe_write(pidPipe[1], (const char *)&doubleForkPid, sizeof(pid_t));
        if (QT_CHDIR("/") == -1)
            qWarning("QProcessPrivate::startDetached: failed to chdir to /");
        ::_exit(1);
    }

    qt_safe_close(startedPipe[1]);
    qt_safe_close(pidPipe[1]);

    if (childPid == -1) {
        qt_safe_close(startedPipe[0]);
        qt_safe_close(pidPipe[0]);
        return false;
    }

    char reply = '\0';
    int startResult = qt_safe_read(startedPipe[0], &reply, 1);
    int result;
    qt_safe_close(startedPipe[0]);
    qt_safe_waitpid(childPid, &result, 0);
    bool success = (startResult != -1 && reply == '\0');
    if (success && pid) {
        pid_t actualPid = 0;
        if (qt_safe_read(pidPipe[0], (char *)&actualPid, sizeof(pid_t)) == sizeof(pid_t)) {
            *pid = actualPid;
        } else {
            *pid = 0;
        }
    }
    qt_safe_close(pidPipe[0]);
    return success;
}

QT_END_NAMESPACE

#endif // QT_NO_PROCESS
