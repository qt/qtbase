// Copyright (C) 2020 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// Copyright (C) 2021 Alex Trotsenko.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

//#define QPROCESS_DEBUG
#include "qdebug.h"
#include <private/qdebug_p.h>
#include "qplatformdefs.h"

#include "qprocess.h"
#include "qprocess_p.h"
#include "qstandardpaths.h"
#include "private/qcore_unix_p.h"
#include "private/qlocking_p.h"

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
#include <qsocketnotifier.h>
#include <qthread.h>

#ifdef Q_OS_QNX
#  include <sys/neutrino.h>
#endif

#include <errno.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>

#if QT_CONFIG(process)
#include <forkfd.h>
#endif

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

#if !defined(Q_OS_DARWIN)

QT_BEGIN_INCLUDE_NAMESPACE
extern char **environ;
QT_END_INCLUDE_NAMESPACE

QProcessEnvironment QProcessEnvironment::systemEnvironment()
{
    QProcessEnvironment env;
    const char *entry;
    for (int count = 0; (entry = environ[count]); ++count) {
        const char *equal = strchr(entry, '=');
        if (!equal)
            continue;

        QByteArray name(entry, equal - entry);
        QByteArray value(equal + 1);
        env.d->vars.insert(QProcessEnvironmentPrivate::Key(name),
                           QProcessEnvironmentPrivate::Value(value));
    }
    return env;
}

#endif // !defined(Q_OS_DARWIN)

#if QT_CONFIG(process)

namespace {
struct AutoPipe
{
    int pipe[2] = { -1, -1 };
    AutoPipe(int flags = 0)
    {
        qt_safe_pipe(pipe, flags);
    }
    ~AutoPipe()
    {
        for (int fd : pipe) {
            if (fd >= 0)
                qt_safe_close(fd);
        }
    }

    explicit operator bool() const  { return pipe[0] >= 0; }
    int &operator[](int idx)        { return pipe[idx]; }
    int operator[](int idx) const   { return pipe[idx]; }
};

struct ChildError
{
    qint64 code;
    char function[8];
};

// Used for argv and envp arguments to execve()
struct CharPointerList
{
    std::unique_ptr<char *[]> pointers;

    CharPointerList(const QString &argv0, const QStringList &args);
    explicit CharPointerList(const QProcessEnvironmentPrivate *env);

private:
    QByteArray data;
    void updatePointers(qsizetype count);
};

struct QProcessPoller
{
    QProcessPoller(const QProcessPrivate &proc);

    int poll(const QDeadlineTimer &deadline);

    pollfd &stdinPipe() { return pfds[0]; }
    pollfd &stdoutPipe() { return pfds[1]; }
    pollfd &stderrPipe() { return pfds[2]; }
    pollfd &forkfd() { return pfds[3]; }

    enum { n_pfds = 4 };
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
}

int QProcessPoller::poll(const QDeadlineTimer &deadline)
{
    return qt_poll_msecs(pfds, n_pfds, deadline.remainingTime());
}

CharPointerList::CharPointerList(const QString &program, const QStringList &args)
{
    qsizetype count = 1 + args.size();
    pointers.reset(new char *[count + 1]);
    pointers[count] = nullptr;

    // we abuse the pointer array to store offsets first (QByteArray will
    // reallocate, after all)
    pointers[0] = reinterpret_cast<char *>(0);
    data = QFile::encodeName(program);
    data += '\0';

    const auto end = args.end();
    auto it = args.begin();
    for (qsizetype i = 1; it != end; ++it, ++i) {
        pointers[i] = reinterpret_cast<char *>(data.size());
        data += QFile::encodeName(*it);
        data += '\0';
    }

    updatePointers(count);
}

CharPointerList::CharPointerList(const QProcessEnvironmentPrivate *environment)
{
    if (!environment)
        return;

    const QProcessEnvironmentPrivate::Map &env = environment->vars;
    qsizetype count = env.size();
    pointers.reset(new char *[count + 1]);
    pointers[count] = nullptr;

    const auto end = env.end();
    auto it = env.begin();
    for (qsizetype i = 0; it != end; ++it, ++i) {
        // we abuse the pointer array to store offsets first (QByteArray will
        // reallocate, after all)
        pointers[i] = reinterpret_cast<char *>(data.size());

        data += it.key();
        data += '=';
        data += it->bytes();
        data += '\0';
    }

    updatePointers(count);
}

void CharPointerList::updatePointers(qsizetype count)
{
    char *const base = const_cast<char *>(data.constBegin());
    for (qsizetype i = 0; i < count; ++i)
        pointers[i] = base + qptrdiff(pointers[i]);
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
        qErrnoWarning("QProcessPrivate::createPipe: Cannot create pipe %p", pipe);
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
    delete channel->notifier;
    channel->notifier = nullptr;

    destroyPipe(channel->pipe);
}

void QProcessPrivate::cleanup()
{
    q_func()->setProcessState(QProcess::NotRunning);

    closeChannels();
    delete stateNotifier;
    stateNotifier = nullptr;
    destroyPipe(childStartedPipe);
    pid = 0;
    if (forkfd != -1) {
        qt_safe_close(forkfd);
        forkfd = -1;
    }
}

/*
    Create the pipes to a QProcessPrivate::Channel.
*/
bool QProcessPrivate::openChannel(Channel &channel)
{
    Q_Q(QProcess);

    if (channel.type == Channel::Normal) {
        // we're piping this channel to our own process
        if (qt_create_pipe(channel.pipe) != 0)
            return false;

        // create the socket notifiers
        if (threadData.loadRelaxed()->hasEventDispatcher()) {
            if (&channel == &stdinChannel) {
                channel.notifier = new QSocketNotifier(QSocketNotifier::Write, q);
                channel.notifier->setSocket(channel.pipe[1]);
                QObject::connect(channel.notifier, SIGNAL(activated(QSocketDescriptor)),
                                 q, SLOT(_q_canWrite()));
            } else {
                channel.notifier = new QSocketNotifier(QSocketNotifier::Read, q);
                channel.notifier->setSocket(channel.pipe[0]);
                const char *receiver;
                if (&channel == &stdoutChannel)
                    receiver = SLOT(_q_canReadStandardOutput());
                else
                    receiver = SLOT(_q_canReadStandardError());
                QObject::connect(channel.notifier, SIGNAL(activated(QSocketDescriptor)),
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

void QProcessPrivate::commitChannels()
{
    // copy the stdin socket if asked to (without closing on exec)
    if (stdinChannel.pipe[0] != INVALID_Q_PIPE)
        qt_safe_dup2(stdinChannel.pipe[0], STDIN_FILENO, 0);

    // copy the stdout and stderr if asked to
    if (stdoutChannel.pipe[1] != INVALID_Q_PIPE)
        qt_safe_dup2(stdoutChannel.pipe[1], STDOUT_FILENO, 0);
    if (stderrChannel.pipe[1] != INVALID_Q_PIPE) {
        qt_safe_dup2(stderrChannel.pipe[1], STDERR_FILENO, 0);
    } else {
        // merge stdout and stderr if asked to
        if (processChannelMode == QProcess::MergedChannels)
            qt_safe_dup2(STDOUT_FILENO, STDERR_FILENO, 0);
    }
}

static QString resolveExecutable(const QString &program)
{
#ifdef Q_OS_DARWIN
    // allow invoking of .app bundles on the Mac.
    QFileInfo fileInfo(program);
    if (program.endsWith(".app"_L1) && fileInfo.isDir()) {
        QCFType<CFURLRef> url = CFURLCreateWithFileSystemPath(0,
                                                          QCFString(fileInfo.absoluteFilePath()),
                                                          kCFURLPOSIXPathStyle, true);
        {
            // CFBundle is not reentrant, since CFBundleCreate might return a reference
            // to a cached bundle object. Protect the bundle calls with a mutex lock.
            Q_CONSTINIT static QBasicMutex cfbundleMutex;
            const auto locker = qt_scoped_lock(cfbundleMutex);
            QCFType<CFBundleRef> bundle = CFBundleCreate(0, url);
            // 'executableURL' can be either relative or absolute ...
            QCFType<CFURLRef> executableURL = CFBundleCopyExecutableURL(bundle);
            // not to depend on caching - make sure it's always absolute.
            url = CFURLCopyAbsoluteURL(executableURL);
        }
        if (url) {
            const QCFString str = CFURLCopyFileSystemPath(url, kCFURLPOSIXPathStyle);
            return QString::fromCFString(str);
        }
    }
#endif

    if (!program.contains(u'/')) {
        // findExecutable() returns its argument if it's an absolute path,
        // otherwise it searches $PATH; returns empty if not found (we handle
        // that case much later)
        return QStandardPaths::findExecutable(program);
    }
    return program;
}

void QProcessPrivate::startProcess()
{
    Q_Q(QProcess);

#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::startProcess()");
#endif

    // Initialize pipes
    if (!openChannels() || qt_create_pipe(childStartedPipe) != 0) {
        setErrorAndEmit(QProcess::FailedToStart, qt_error_string(errno));
        cleanup();
        return;
    }

    if (threadData.loadRelaxed()->hasEventDispatcher()) {
        // Set up to notify about startup completion (and premature death).
        // Once the process has started successfully, we reconfigure the
        // notifier to watch the fork_fd for expected death.
        stateNotifier = new QSocketNotifier(childStartedPipe[0],
                                            QSocketNotifier::Read, q);
        QObject::connect(stateNotifier, SIGNAL(activated(QSocketDescriptor)),
                         q, SLOT(_q_startupNotification()));
    }

    // Start the process (platform dependent)
    q->setProcessState(QProcess::Starting);

    // Prepare the arguments and the environment
    const CharPointerList argv(resolveExecutable(program), arguments);
    const CharPointerList envp(environment.d.constData());

    // Encode the working directory if it's non-empty, otherwise just pass 0.
    const char *workingDirPtr = nullptr;
    QByteArray encodedWorkingDirectory;
    if (!workingDirectory.isEmpty()) {
        encodedWorkingDirectory = QFile::encodeName(workingDirectory);
        workingDirPtr = encodedWorkingDirectory.constData();
    }

    // Start the child.
    auto execChild1 = [this, workingDirPtr, &argv, &envp]() {
        execChild(workingDirPtr, argv.pointers.get(), envp.pointers.get());
    };
    auto execChild2 = [](void *lambda) {
        static_cast<decltype(execChild1) *>(lambda)->operator()();
        return -1;
    };

    int ffdflags = FFD_CLOEXEC;

    // QTBUG-86285
#if defined(Q_OS_LINUX) && !QT_CONFIG(forkfd_pidfd)
    ffdflags |= FFD_USE_FORK;
#endif

    pid_t childPid;
    forkfd = ::vforkfd(ffdflags , &childPid, execChild2, &execChild1);
    int lastForkErrno = errno;

    if (forkfd == -1) {
        // Cleanup, report error and return
#if defined (QPROCESS_DEBUG)
        qDebug("fork failed: %ls", qUtf16Printable(qt_error_string(lastForkErrno)));
#endif
        q->setProcessState(QProcess::NotRunning);
        setErrorAndEmit(QProcess::FailedToStart,
                        QProcess::tr("Resource error (fork failure): %1").arg(qt_error_string(lastForkErrno)));
        cleanup();
        return;
    }

    pid = qint64(childPid);
    Q_ASSERT(pid > 0);

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
}

void QProcessPrivate::execChild(const char *workingDir, char **argv, char **envp)
{
    ::signal(SIGPIPE, SIG_DFL);         // reset the signal that we ignored

    ChildError error = { 0, {} };       // force zeroing of function[8]

    // Render channels configuration.
    commitChannels();

    // make sure this fd is closed if execv() succeeds
    qt_safe_close(childStartedPipe[0]);

    // enter the working directory
    if (workingDir && QT_CHDIR(workingDir) == -1) {
        // failed, stop the process
        strcpy(error.function, "chdir");
        goto report_errno;
    }

    if (childProcessModifier)
        childProcessModifier();

    // execute the process
    if (!envp) {
        qt_safe_execv(argv[0], argv);
        strcpy(error.function, "execvp");
    } else {
#if defined (QPROCESS_DEBUG)
        fprintf(stderr, "QProcessPrivate::execChild() starting %s\n", argv[0]);
#endif
        qt_safe_execve(argv[0], argv, envp);
        strcpy(error.function, "execve");
    }

    // notify failure
    // don't use strerror or any other routines that may allocate memory, since
    // some buggy libc versions can deadlock on locked mutexes.
report_errno:
    error.code = errno;
    qt_safe_write(childStartedPipe[1], &error, sizeof(error));
    childStartedPipe[1] = -1;
}

bool QProcessPrivate::processStarted(QString *errorMessage)
{
    Q_Q(QProcess);

    ChildError buf;
    int ret = qt_safe_read(childStartedPipe[0], &buf, sizeof(buf));

    if (stateNotifier) {
        stateNotifier->setEnabled(false);
        stateNotifier->disconnect(q);
    }
    qt_safe_close(childStartedPipe[0]);
    childStartedPipe[0] = -1;

#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::processStarted() == %s", ret <= 0 ? "true" : "false");
#endif

    if (ret <= 0) {  // process successfully started
        if (stateNotifier) {
            QObject::connect(stateNotifier, SIGNAL(activated(QSocketDescriptor)),
                             q, SLOT(_q_processDied()));
            stateNotifier->setSocket(forkfd);
            stateNotifier->setEnabled(true);
        }
        if (stdoutChannel.notifier)
            stdoutChannel.notifier->setEnabled(true);
        if (stderrChannel.notifier)
            stderrChannel.notifier->setEnabled(true);

        return true;
    }

    // did we read an error message?
    if (errorMessage)
        *errorMessage = QLatin1StringView(buf.function) + ": "_L1 + qt_error_string(buf.code);

    return false;
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
           data, QtDebugUtils::toPrintable(data, bytesRead, 16).constData(), maxlen, bytesRead);
    errno = save_errno;
#endif
    if (bytesRead == -1 && errno == EWOULDBLOCK)
        return -2;
    return bytesRead;
}

/*! \reimp
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

    d->write(data, len);
    if (d->stdinChannel.notifier)
        d->stdinChannel.notifier->setEnabled(true);

#if defined QPROCESS_DEBUG
    qDebug("QProcess::writeData(%p \"%s\", %lld) == %lld (written to buffer)",
           data, QtDebugUtils::toPrintable(data, len, 16).constData(), len, len);
#endif
    return len;
}

bool QProcessPrivate::_q_canWrite()
{
    if (writeBuffer.isEmpty()) {
        if (stdinChannel.notifier)
            stdinChannel.notifier->setEnabled(false);
#if defined QPROCESS_DEBUG
        qDebug("QProcessPrivate::canWrite(), not writing anything (empty write buffer).");
#endif
        return false;
    }

    const bool writeSucceeded = writeToStdin();

    if (writeBuffer.isEmpty() && stdinChannel.closed)
        closeWriteChannel();
    else if (stdinChannel.notifier)
        stdinChannel.notifier->setEnabled(!writeBuffer.isEmpty());

    return writeSucceeded;
}

bool QProcessPrivate::writeToStdin()
{
    const char *data = writeBuffer.readPointer();
    const qint64 bytesToWrite = writeBuffer.nextDataBlockSize();

    qint64 written = qt_safe_write_nosignal(stdinChannel.pipe[1], data, bytesToWrite);
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::writeToStdin(), write(%p \"%s\", %lld) == %lld", data,
           QtDebugUtils::toPrintable(data, bytesToWrite, 16).constData(), bytesToWrite, written);
    if (written == -1)
        qDebug("QProcessPrivate::writeToStdin(), failed to write (%ls)", qUtf16Printable(qt_error_string(errno)));
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
    qDebug("QProcessPrivate::terminateProcess() pid=%jd", intmax_t(pid));
#endif
    if (pid > 0)
        ::kill(pid_t(pid), SIGTERM);
}

void QProcessPrivate::killProcess()
{
#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::killProcess() pid=%jd", intmax_t(pid));
#endif
    if (pid > 0)
        ::kill(pid_t(pid), SIGKILL);
}

bool QProcessPrivate::waitForStarted(const QDeadlineTimer &deadline)
{
    const qint64 msecs = deadline.remainingTime();
#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::waitForStarted(%lld) waiting for child to start (fd = %d)",
           msecs, childStartedPipe[0]);
#endif

    pollfd pfd = qt_make_pollfd(childStartedPipe[0], POLLIN);

    if (qt_poll_msecs(&pfd, 1, msecs) == 0) {
        setError(QProcess::Timedout);
#if defined (QPROCESS_DEBUG)
        qDebug("QProcessPrivate::waitForStarted(%lld) == false (timed out)", msecs);
#endif
        return false;
    }

    bool startedEmitted = _q_startupNotification();
#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::waitForStarted() == %s", startedEmitted ? "true" : "false");
#endif
    return startedEmitted;
}

bool QProcessPrivate::waitForReadyRead(const QDeadlineTimer &deadline)
{
#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::waitForReadyRead(%lld)", deadline.remainingTime());
#endif

    forever {
        QProcessPoller poller(*this);

        int ret = poller.poll(deadline);

        if (ret < 0) {
            break;
        }
        if (ret == 0) {
            setError(QProcess::Timedout);
            return false;
        }

        // This calls QProcessPrivate::tryReadFromChannel(), which returns true
        // if we emitted readyRead() signal on the current read channel.
        bool readyReadEmitted = false;
        if (qt_pollfd_check(poller.stdoutPipe(), POLLIN) && _q_canReadStandardOutput())
            readyReadEmitted = true;
        if (qt_pollfd_check(poller.stderrPipe(), POLLIN) && _q_canReadStandardError())
            readyReadEmitted = true;

        if (readyReadEmitted)
            return true;

        if (qt_pollfd_check(poller.stdinPipe(), POLLOUT))
            _q_canWrite();

        // Signals triggered by I/O may have stopped this process:
        if (processState == QProcess::NotRunning)
            return false;

        // We do this after checking the pipes, so we cannot reach it as long
        // as there is any data left to be read from an already dead process.
        if (qt_pollfd_check(poller.forkfd(), POLLIN)) {
            processFinished();
            return false;
        }
    }
    return false;
}

bool QProcessPrivate::waitForBytesWritten(const QDeadlineTimer &deadline)
{
#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::waitForBytesWritten(%lld)", deadline.remainingTime());
#endif

    while (!writeBuffer.isEmpty()) {
        QProcessPoller poller(*this);

        int ret = poller.poll(deadline);

        if (ret < 0) {
            break;
        }

        if (ret == 0) {
            setError(QProcess::Timedout);
            return false;
        }

        if (qt_pollfd_check(poller.stdinPipe(), POLLOUT))
            return _q_canWrite();

        if (qt_pollfd_check(poller.stdoutPipe(), POLLIN))
            _q_canReadStandardOutput();

        if (qt_pollfd_check(poller.stderrPipe(), POLLIN))
            _q_canReadStandardError();

        // Signals triggered by I/O may have stopped this process:
        if (processState == QProcess::NotRunning)
            return false;

        if (qt_pollfd_check(poller.forkfd(), POLLIN)) {
            processFinished();
            return false;
        }
    }

    return false;
}

bool QProcessPrivate::waitForFinished(const QDeadlineTimer &deadline)
{
#if defined (QPROCESS_DEBUG)
    qDebug("QProcessPrivate::waitForFinished(%lld)", deadline.remainingTime());
#endif

    forever {
        QProcessPoller poller(*this);

        int ret = poller.poll(deadline);

        if (ret < 0) {
            break;
        }
        if (ret == 0) {
            setError(QProcess::Timedout);
            return false;
        }

        if (qt_pollfd_check(poller.stdinPipe(), POLLOUT))
            _q_canWrite();

        if (qt_pollfd_check(poller.stdoutPipe(), POLLIN))
            _q_canReadStandardOutput();

        if (qt_pollfd_check(poller.stderrPipe(), POLLIN))
            _q_canReadStandardError();

        // Signals triggered by I/O may have stopped this process:
        if (processState == QProcess::NotRunning)
            return true;

        if (qt_pollfd_check(poller.forkfd(), POLLIN)) {
            processFinished();
            return true;
        }
    }
    return false;
}

void QProcessPrivate::waitForDeadChild()
{
    Q_ASSERT(forkfd != -1);

    // read the process information from our fd
    forkfd_info info;
    int ret;
    EINTR_LOOP(ret, forkfd_wait(forkfd, &info, nullptr));

    exitCode = info.status;
    crashed = info.code != CLD_EXITED;

    delete stateNotifier;
    stateNotifier = nullptr;

    EINTR_LOOP(ret, forkfd_close(forkfd));
    forkfd = -1; // Child is dead, don't try to kill it anymore

#if defined QPROCESS_DEBUG
    qDebug() << "QProcessPrivate::waitForDeadChild() dead with exitCode"
             << exitCode << ", crashed?" << crashed;
#endif
}

bool QProcessPrivate::startDetached(qint64 *pid)
{
    QByteArray encodedWorkingDirectory = QFile::encodeName(workingDirectory);

#ifdef PIPE_BUF
    static_assert(PIPE_BUF >= sizeof(ChildError));
#else
    static_assert(_POSIX_PIPE_BUF >= sizeof(ChildError));
#endif
    ChildError childStatus = { 0, {} };

    AutoPipe startedPipe, pidPipe;
    if (!startedPipe || !pidPipe) {
        setErrorAndEmit(QProcess::FailedToStart, "pipe: "_L1 + qt_error_string(errno));
        return false;
    }

    if (!openChannelsForDetached()) {
        // openChannel sets the error string
        closeChannels();
        return false;
    }

    const CharPointerList argv(resolveExecutable(program), arguments);
    const CharPointerList envp(environment.d.constData());

    pid_t childPid = fork();
    if (childPid == 0) {
        ::signal(SIGPIPE, SIG_DFL);     // reset the signal that we ignored
        ::setsid();

        qt_safe_close(startedPipe[0]);
        qt_safe_close(pidPipe[0]);

        auto reportFailed = [&](const char *function) {
            childStatus.code = errno;
            strcpy(childStatus.function, function);
            qt_safe_write(startedPipe[1], &childStatus, sizeof(childStatus));
            ::_exit(1);
        };

        if (!encodedWorkingDirectory.isEmpty()) {
            if (QT_CHDIR(encodedWorkingDirectory.constData()) < 0)
                reportFailed("chdir: ");
        }

        pid_t doubleForkPid = fork();
        if (doubleForkPid == 0) {
            // Render channels configuration.
            commitChannels();

            if (envp.pointers)
                qt_safe_execve(argv.pointers[0], argv.pointers.get(), envp.pointers.get());
            else
                qt_safe_execv(argv.pointers[0], argv.pointers.get());

            reportFailed("execv: ");
        } else if (doubleForkPid == -1) {
            reportFailed("fork: ");
        }

        // success
        qt_safe_write(pidPipe[1], &doubleForkPid, sizeof(pid_t));
        ::_exit(1);
    }

    int savedErrno = errno;
    closeChannels();

    if (childPid == -1) {
        setErrorAndEmit(QProcess::FailedToStart, "fork: "_L1 + qt_error_string(savedErrno));
        return false;
    }

    // close the writing ends of the pipes so we can properly get EOFs
    qt_safe_close(pidPipe[1]);
    qt_safe_close(startedPipe[1]);
    pidPipe[1] = startedPipe[1] = -1;

    // This read() will block until we're cleared to proceed. If it returns 0
    // (EOF), it means the direct child has exited and the grandchild
    // successfully execve()'d the target process. If it returns any positive
    // result, it means one of the two children wrote an error result. Negative
    // values should not happen.
    ssize_t startResult = qt_safe_read(startedPipe[0], &childStatus, sizeof(childStatus));

    // reap the intermediate child
    int result;
    qt_safe_waitpid(childPid, &result, 0);

    bool success = (startResult == 0);  // nothing written -> no error
    if (success && pid) {
        pid_t actualPid;
        if (qt_safe_read(pidPipe[0], &actualPid, sizeof(pid_t)) != sizeof(pid_t))
            actualPid = 0;              // this shouldn't happen!
        *pid = actualPid;
    } else if (!success) {
        if (pid)
            *pid = -1;
        QString msg;
        if (startResult == sizeof(childStatus))
            msg = QLatin1StringView(childStatus.function) + qt_error_string(childStatus.code);
        setErrorAndEmit(QProcess::FailedToStart, msg);
    }
    return success;
}

#endif // QT_CONFIG(process)

QT_END_NAMESPACE
