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

#include <qdebug.h>
#include <qdir.h>
#if defined(Q_OS_WIN)
#include <qtimer.h>
#endif
#if defined QPROCESS_DEBUG
#include <qstring.h>
#include <ctype.h>

QT_BEGIN_NAMESPACE
/*
    Returns a human readable representation of the first \a len
    characters in \a data.
*/
static QByteArray qt_prettyDebug(const char *data, int len, int maxSize)
{
    if (!data) return "(null)";
    QByteArray out;
    for (int i = 0; i < len && i < maxSize; ++i) {
        char c = data[i];
        if (isprint(c)) {
            out += c;
        } else switch (c) {
        case '\n': out += "\\n"; break;
        case '\r': out += "\\r"; break;
        case '\t': out += "\\t"; break;
        default:
            char buf[5];
            qsnprintf(buf, sizeof(buf), "\\%3o", c);
            buf[4] = '\0';
            out += QByteArray(buf);
        }
    }

    if (len < maxSize)
        out += "...";

    return out;
}

QT_END_NAMESPACE

#endif

#include "qprocess.h"
#include "qprocess_p.h"

#include <qbytearray.h>
#include <qelapsedtimer.h>
#include <qcoreapplication.h>
#include <qsocketnotifier.h>
#include <qtimer.h>

#ifdef Q_OS_WIN
#include <qwineventnotifier.h>
#else
#include <private/qcore_unix_p.h>
#endif

#if QT_HAS_INCLUDE(<paths.h>)
#include <paths.h>
#endif

QT_BEGIN_NAMESPACE

/*!
    \since 5.6

    \macro QT_NO_PROCESS_COMBINED_ARGUMENT_START
    \relates QProcess

    Disables the
    \l {QProcess::start(const QString &, OpenMode)}{QProcess::start()}
    overload taking a single string.
    In most cases where it is used, the user intends for the first argument
    to be treated atomically as per the other overload.

    \sa QProcess::start(const QString &command, OpenMode mode)
*/

/*!
    \class QProcessEnvironment
    \inmodule QtCore

    \brief The QProcessEnvironment class holds the environment variables that
    can be passed to a program.

    \ingroup io
    \ingroup misc
    \ingroup shared
    \reentrant
    \since 4.6

    A process's environment is composed of a set of key=value pairs known as
    environment variables. The QProcessEnvironment class wraps that concept
    and allows easy manipulation of those variables. It's meant to be used
    along with QProcess, to set the environment for child processes. It
    cannot be used to change the current process's environment.

    The environment of the calling process can be obtained using
    QProcessEnvironment::systemEnvironment().

    On Unix systems, the variable names are case-sensitive. Note that the
    Unix environment allows both variable names and contents to contain arbitrary
    binary data (except for the NUL character). QProcessEnvironment will preserve
    such variables, but does not support manipulating variables whose names or
    values cannot be encoded by the current locale settings (see
    QTextCodec::codecForLocale).

    On Windows, the variable names are case-insensitive, but case-preserving.
    QProcessEnvironment behaves accordingly.

    \sa QProcess, QProcess::systemEnvironment(), QProcess::setProcessEnvironment()
*/

QStringList QProcessEnvironmentPrivate::toList() const
{
    QStringList result;
    result.reserve(vars.size());
    for (auto it = vars.cbegin(), end = vars.cend(); it != end; ++it)
        result << nameToString(it.key()) + QLatin1Char('=') + valueToString(it.value());
    return result;
}

QProcessEnvironment QProcessEnvironmentPrivate::fromList(const QStringList &list)
{
    QProcessEnvironment env;
    QStringList::ConstIterator it = list.constBegin(),
                              end = list.constEnd();
    for ( ; it != end; ++it) {
        int pos = it->indexOf(QLatin1Char('='), 1);
        if (pos < 1)
            continue;

        QString value = it->mid(pos + 1);
        QString name = *it;
        name.truncate(pos);
        env.insert(name, value);
    }
    return env;
}

QStringList QProcessEnvironmentPrivate::keys() const
{
    QStringList result;
    result.reserve(vars.size());
    auto it = vars.constBegin();
    const auto end = vars.constEnd();
    for ( ; it != end; ++it)
        result << nameToString(it.key());
    return result;
}

void QProcessEnvironmentPrivate::insert(const QProcessEnvironmentPrivate &other)
{
    auto it = other.vars.constBegin();
    const auto end = other.vars.constEnd();
    for ( ; it != end; ++it)
        vars.insert(it.key(), it.value());

#ifdef Q_OS_UNIX
    auto nit = other.nameMap.constBegin();
    const auto nend = other.nameMap.constEnd();
    for ( ; nit != nend; ++nit)
        nameMap.insert(nit.key(), nit.value());
#endif
}

/*!
    Creates a new QProcessEnvironment object. This constructor creates an
    empty environment. If set on a QProcess, this will cause the current
    environment variables to be removed.
*/
QProcessEnvironment::QProcessEnvironment()
    : d(0)
{
}

/*!
    Frees the resources associated with this QProcessEnvironment object.
*/
QProcessEnvironment::~QProcessEnvironment()
{
}

/*!
    Creates a QProcessEnvironment object that is a copy of \a other.
*/
QProcessEnvironment::QProcessEnvironment(const QProcessEnvironment &other)
    : d(other.d)
{
}

/*!
    Copies the contents of the \a other QProcessEnvironment object into this
    one.
*/
QProcessEnvironment &QProcessEnvironment::operator=(const QProcessEnvironment &other)
{
    d = other.d;
    return *this;
}

/*!
    \fn void QProcessEnvironment::swap(QProcessEnvironment &other)
    \since 5.0

    Swaps this process environment instance with \a other. This
    function is very fast and never fails.
*/

/*!
    \fn bool QProcessEnvironment::operator !=(const QProcessEnvironment &other) const

    Returns \c true if this and the \a other QProcessEnvironment objects are different.

    \sa operator==()
*/

/*!
    Returns \c true if this and the \a other QProcessEnvironment objects are equal.

    Two QProcessEnvironment objects are considered equal if they have the same
    set of key=value pairs. The comparison of keys is done case-sensitive on
    platforms where the environment is case-sensitive.

    \sa operator!=(), contains()
*/
bool QProcessEnvironment::operator==(const QProcessEnvironment &other) const
{
    if (d == other.d)
        return true;
    if (d) {
        if (other.d) {
            QProcessEnvironmentPrivate::OrderedMutexLocker locker(d, other.d);
            return d->vars == other.d->vars;
        } else {
            return isEmpty();
        }
    } else {
        return other.isEmpty();
    }
}

/*!
    Returns \c true if this QProcessEnvironment object is empty: that is
    there are no key=value pairs set.

    \sa clear(), systemEnvironment(), insert()
*/
bool QProcessEnvironment::isEmpty() const
{
    // Needs no locking, as no hash nodes are accessed
    return d ? d->vars.isEmpty() : true;
}

/*!
    Removes all key=value pairs from this QProcessEnvironment object, making
    it empty.

    \sa isEmpty(), systemEnvironment()
*/
void QProcessEnvironment::clear()
{
    if (d)
        d->vars.clear();
    // Unix: Don't clear d->nameMap, as the environment is likely to be
    // re-populated with the same keys again.
}

/*!
    Returns \c true if the environment variable of name \a name is found in
    this QProcessEnvironment object.


    \sa insert(), value()
*/
bool QProcessEnvironment::contains(const QString &name) const
{
    if (!d)
        return false;
    QProcessEnvironmentPrivate::MutexLocker locker(d);
    return d->vars.contains(d->prepareName(name));
}

/*!
    Inserts the environment variable of name \a name and contents \a value
    into this QProcessEnvironment object. If that variable already existed,
    it is replaced by the new value.

    On most systems, inserting a variable with no contents will have the
    same effect for applications as if the variable had not been set at all.
    However, to guarantee that there are no incompatibilities, to remove a
    variable, please use the remove() function.

    \sa contains(), remove(), value()
*/
void QProcessEnvironment::insert(const QString &name, const QString &value)
{
    // our re-impl of detach() detaches from null
    d.detach(); // detach before prepareName()
    d->vars.insert(d->prepareName(name), d->prepareValue(value));
}

/*!
    Removes the environment variable identified by \a name from this
    QProcessEnvironment object. If that variable did not exist before,
    nothing happens.


    \sa contains(), insert(), value()
*/
void QProcessEnvironment::remove(const QString &name)
{
    if (d) {
        d.detach(); // detach before prepareName()
        d->vars.remove(d->prepareName(name));
    }
}

/*!
    Searches this QProcessEnvironment object for a variable identified by
    \a name and returns its value. If the variable is not found in this object,
    then \a defaultValue is returned instead.

    \sa contains(), insert(), remove()
*/
QString QProcessEnvironment::value(const QString &name, const QString &defaultValue) const
{
    if (!d)
        return defaultValue;

    QProcessEnvironmentPrivate::MutexLocker locker(d);
    const auto it = d->vars.constFind(d->prepareName(name));
    if (it == d->vars.constEnd())
        return defaultValue;

    return d->valueToString(it.value());
}

/*!
    Converts this QProcessEnvironment object into a list of strings, one for
    each environment variable that is set. The environment variable's name
    and its value are separated by an equal character ('=').

    The QStringList contents returned by this function are suitable for
    presentation.
    Use with the QProcess::setEnvironment function is not recommended due to
    potential encoding problems under Unix, and worse performance.

    \sa systemEnvironment(), QProcess::systemEnvironment(),
        QProcess::setProcessEnvironment()
*/
QStringList QProcessEnvironment::toStringList() const
{
    if (!d)
        return QStringList();
    QProcessEnvironmentPrivate::MutexLocker locker(d);
    return d->toList();
}

/*!
    \since 4.8

    Returns a list containing all the variable names in this QProcessEnvironment
    object.
*/
QStringList QProcessEnvironment::keys() const
{
    if (!d)
        return QStringList();
    QProcessEnvironmentPrivate::MutexLocker locker(d);
    return d->keys();
}

/*!
    \overload
    \since 4.8

    Inserts the contents of \a e in this QProcessEnvironment object. Variables in
    this object that also exist in \a e will be overwritten.
*/
void QProcessEnvironment::insert(const QProcessEnvironment &e)
{
    if (!e.d)
        return;

    // our re-impl of detach() detaches from null
    QProcessEnvironmentPrivate::MutexLocker locker(e.d);
    d->insert(*e.d);
}

#if QT_CONFIG(process)

void QProcessPrivate::Channel::clear()
{
    switch (type) {
    case PipeSource:
        Q_ASSERT(process);
        process->stdinChannel.type = Normal;
        process->stdinChannel.process = 0;
        break;
    case PipeSink:
        Q_ASSERT(process);
        process->stdoutChannel.type = Normal;
        process->stdoutChannel.process = 0;
        break;
    }

    type = Normal;
    file.clear();
    process = 0;
}

/*!
    \class QProcess
    \inmodule QtCore

    \brief The QProcess class is used to start external programs and
    to communicate with them.

    \ingroup io

    \reentrant

    \section1 Running a Process

    To start a process, pass the name and command line arguments of
    the program you want to run as arguments to start(). Arguments
    are supplied as individual strings in a QStringList.

    Alternatively, you can set the program to run with setProgram()
    and setArguments(), and then call start() or open().

    For example, the following code snippet runs the analog clock
    example in the Fusion style on X11 platforms by passing strings
    containing "-style" and "fusion" as two items in the list of
    arguments:

    \snippet qprocess/qprocess-simpleexecution.cpp 0
    \dots
    \snippet qprocess/qprocess-simpleexecution.cpp 1
    \snippet qprocess/qprocess-simpleexecution.cpp 2

    QProcess then enters the \l Starting state, and when the program
    has started, QProcess enters the \l Running state and emits
    started().

    QProcess allows you to treat a process as a sequential I/O
    device. You can write to and read from the process just as you
    would access a network connection using QTcpSocket. You can then
    write to the process's standard input by calling write(), and
    read the standard output by calling read(), readLine(), and
    getChar(). Because it inherits QIODevice, QProcess can also be
    used as an input source for QXmlReader, or for generating data to
    be uploaded using QNetworkAccessManager.

    When the process exits, QProcess reenters the \l NotRunning state
    (the initial state), and emits finished().

    The finished() signal provides the exit code and exit status of
    the process as arguments, and you can also call exitCode() to
    obtain the exit code of the last process that finished, and
    exitStatus() to obtain its exit status. If an error occurs at
    any point in time, QProcess will emit the errorOccurred() signal.
    You can also call error() to find the type of error that occurred
    last, and state() to find the current process state.

    \note QProcess is not supported on VxWorks, iOS, tvOS, watchOS,
    or the Universal Windows Platform.

    \section1 Communicating via Channels

    Processes have two predefined output channels: The standard
    output channel (\c stdout) supplies regular console output, and
    the standard error channel (\c stderr) usually supplies the
    errors that are printed by the process. These channels represent
    two separate streams of data. You can toggle between them by
    calling setReadChannel(). QProcess emits readyRead() when data is
    available on the current read channel. It also emits
    readyReadStandardOutput() when new standard output data is
    available, and when new standard error data is available,
    readyReadStandardError() is emitted. Instead of calling read(),
    readLine(), or getChar(), you can explicitly read all data from
    either of the two channels by calling readAllStandardOutput() or
    readAllStandardError().

    The terminology for the channels can be misleading. Be aware that
    the process's output channels correspond to QProcess's
    \e read channels, whereas the process's input channels correspond
    to QProcess's \e write channels. This is because what we read
    using QProcess is the process's output, and what we write becomes
    the process's input.

    QProcess can merge the two output channels, so that standard
    output and standard error data from the running process both use
    the standard output channel. Call setProcessChannelMode() with
    MergedChannels before starting the process to activate
    this feature. You also have the option of forwarding the output of
    the running process to the calling, main process, by passing
    ForwardedChannels as the argument. It is also possible to forward
    only one of the output channels - typically one would use
    ForwardedErrorChannel, but ForwardedOutputChannel also exists.
    Note that using channel forwarding is typically a bad idea in GUI
    applications - you should present errors graphically instead.

    Certain processes need special environment settings in order to
    operate. You can set environment variables for your process by
    calling setProcessEnvironment(). To set a working directory, call
    setWorkingDirectory(). By default, processes are run in the
    current working directory of the calling process.

    The positioning and the screen Z-order of windows belonging to
    GUI applications started with QProcess are controlled by
    the underlying windowing system. For Qt 5 applications, the
    positioning can be specified using the \c{-qwindowgeometry}
    command line option; X11 applications generally accept a
    \c{-geometry} command line option.

    \note On QNX, setting the working directory may cause all
    application threads, with the exception of the QProcess caller
    thread, to temporarily freeze during the spawning process,
    owing to a limitation in the operating system.

    \section1 Synchronous Process API

    QProcess provides a set of functions which allow it to be used
    without an event loop, by suspending the calling thread until
    certain signals are emitted:

    \list
    \li waitForStarted() blocks until the process has started.

    \li waitForReadyRead() blocks until new data is
    available for reading on the current read channel.

    \li waitForBytesWritten() blocks until one payload of
    data has been written to the process.

    \li waitForFinished() blocks until the process has finished.
    \endlist

    Calling these functions from the main thread (the thread that
    calls QApplication::exec()) may cause your user interface to
    freeze.

    The following example runs \c gzip to compress the string "Qt
    rocks!", without an event loop:

    \snippet process/process.cpp 0

    \section1 Notes for Windows Users

    Some Windows commands (for example, \c dir) are not provided by
    separate applications, but by the command interpreter itself.
    If you attempt to use QProcess to execute these commands directly,
    it won't work. One possible solution is to execute the command
    interpreter itself (\c{cmd.exe} on some Windows systems), and ask
    the interpreter to execute the desired command.

    \sa QBuffer, QFile, QTcpSocket
*/

/*!
    \enum QProcess::ProcessChannel

    This enum describes the process channels used by the running process.
    Pass one of these values to setReadChannel() to set the
    current read channel of QProcess.

    \value StandardOutput The standard output (stdout) of the running
           process.

    \value StandardError The standard error (stderr) of the running
           process.

    \sa setReadChannel()
*/

/*!
    \enum QProcess::ProcessChannelMode

    This enum describes the process output channel modes of QProcess.
    Pass one of these values to setProcessChannelMode() to set the
    current read channel mode.

    \value SeparateChannels QProcess manages the output of the
    running process, keeping standard output and standard error data
    in separate internal buffers. You can select the QProcess's
    current read channel by calling setReadChannel(). This is the
    default channel mode of QProcess.

    \value MergedChannels QProcess merges the output of the running
    process into the standard output channel (\c stdout). The
    standard error channel (\c stderr) will not receive any data. The
    standard output and standard error data of the running process
    are interleaved.

    \value ForwardedChannels QProcess forwards the output of the
    running process onto the main process. Anything the child process
    writes to its standard output and standard error will be written
    to the standard output and standard error of the main process.

    \value ForwardedErrorChannel QProcess manages the standard output
    of the running process, but forwards its standard error onto the
    main process. This reflects the typical use of command line tools
    as filters, where the standard output is redirected to another
    process or a file, while standard error is printed to the console
    for diagnostic purposes.
    (This value was introduced in Qt 5.2.)

    \value ForwardedOutputChannel Complementary to ForwardedErrorChannel.
    (This value was introduced in Qt 5.2.)

    \note Windows intentionally suppresses output from GUI-only
    applications to inherited consoles.
    This does \e not apply to output redirected to files or pipes.
    To forward the output of GUI-only applications on the console
    nonetheless, you must use SeparateChannels and do the forwarding
    yourself by reading the output and writing it to the appropriate
    output channels.

    \sa setProcessChannelMode()
*/

/*!
    \enum QProcess::InputChannelMode
    \since 5.2

    This enum describes the process input channel modes of QProcess.
    Pass one of these values to setInputChannelMode() to set the
    current write channel mode.

    \value ManagedInputChannel QProcess manages the input of the running
    process. This is the default input channel mode of QProcess.

    \value ForwardedInputChannel QProcess forwards the input of the main
    process onto the running process. The child process reads its standard
    input from the same source as the main process.
    Note that the main process must not try to read its standard input
    while the child process is running.

    \sa setInputChannelMode()
*/

/*!
    \enum QProcess::ProcessError

    This enum describes the different types of errors that are
    reported by QProcess.

    \value FailedToStart The process failed to start. Either the
    invoked program is missing, or you may have insufficient
    permissions to invoke the program.

    \value Crashed The process crashed some time after starting
    successfully.

    \value Timedout The last waitFor...() function timed out. The
    state of QProcess is unchanged, and you can try calling
    waitFor...() again.

    \value WriteError An error occurred when attempting to write to the
    process. For example, the process may not be running, or it may
    have closed its input channel.

    \value ReadError An error occurred when attempting to read from
    the process. For example, the process may not be running.

    \value UnknownError An unknown error occurred. This is the default
    return value of error().

    \sa error()
*/

/*!
    \enum QProcess::ProcessState

    This enum describes the different states of QProcess.

    \value NotRunning The process is not running.

    \value Starting The process is starting, but the program has not
    yet been invoked.

    \value Running The process is running and is ready for reading and
    writing.

    \sa state()
*/

/*!
    \enum QProcess::ExitStatus

    This enum describes the different exit statuses of QProcess.

    \value NormalExit The process exited normally.

    \value CrashExit The process crashed.

    \sa exitStatus()
*/

/*!
    \typedef QProcess::CreateProcessArgumentModifier
    \note This typedef is only available on desktop Windows.

    On Windows, QProcess uses the Win32 API function \c CreateProcess to
    start child processes. While QProcess provides a comfortable way to start
    processes without worrying about platform
    details, it is in some cases desirable to fine-tune the parameters that are
    passed to \c CreateProcess. This is done by defining a
    \c CreateProcessArgumentModifier function and passing it to
    \c setCreateProcessArgumentsModifier.

    A \c CreateProcessArgumentModifier function takes one parameter: a pointer
    to a \c CreateProcessArguments struct. The members of this struct will be
    passed to \c CreateProcess after the \c CreateProcessArgumentModifier
    function is called.

    The following example demonstrates how to pass custom flags to
    \c CreateProcess.
    When starting a console process B from a console process A, QProcess will
    reuse the console window of process A for process B by default. In this
    example, a new console window with a custom color scheme is created for the
    child process B instead.

    \snippet qprocess/qprocess-createprocessargumentsmodifier.cpp 0

    \sa QProcess::CreateProcessArguments
    \sa setCreateProcessArgumentsModifier()
*/

/*!
    \class QProcess::CreateProcessArguments
    \note This struct is only available on the Windows platform.

    This struct is a representation of all parameters of the Windows API
    function \c CreateProcess. It is used as parameter for
    \c CreateProcessArgumentModifier functions.

    \sa QProcess::CreateProcessArgumentModifier
*/

/*!
    \fn void QProcess::error(QProcess::ProcessError error)
    \obsolete

    Use errorOccurred() instead.
*/

/*!
    \fn void QProcess::errorOccurred(QProcess::ProcessError error)
    \since 5.6

    This signal is emitted when an error occurs with the process. The
    specified \a error describes the type of error that occurred.
*/

/*!
    \fn void QProcess::started()

    This signal is emitted by QProcess when the process has started,
    and state() returns \l Running.
*/

/*!
    \fn void QProcess::stateChanged(QProcess::ProcessState newState)

    This signal is emitted whenever the state of QProcess changes. The
    \a newState argument is the state QProcess changed to.
*/

/*!
    \fn void QProcess::finished(int exitCode)
    \obsolete
    \overload

    Use finished(int exitCode, QProcess::ExitStatus status) instead.
*/

/*!
    \fn void QProcess::finished(int exitCode, QProcess::ExitStatus exitStatus)

    This signal is emitted when the process finishes. \a exitCode is the exit
    code of the process (only valid for normal exits), and \a exitStatus is
    the exit status.
    After the process has finished, the buffers in QProcess are still intact.
    You can still read any data that the process may have written before it
    finished.

    \sa exitStatus()
*/

/*!
    \fn void QProcess::readyReadStandardOutput()

    This signal is emitted when the process has made new data
    available through its standard output channel (\c stdout). It is
    emitted regardless of the current \l{readChannel()}{read channel}.

    \sa readAllStandardOutput(), readChannel()
*/

/*!
    \fn void QProcess::readyReadStandardError()

    This signal is emitted when the process has made new data
    available through its standard error channel (\c stderr). It is
    emitted regardless of the current \l{readChannel()}{read
    channel}.

    \sa readAllStandardError(), readChannel()
*/

/*!
    \internal
*/
QProcessPrivate::QProcessPrivate()
{
    readBufferChunkSize = QRINGBUFFER_CHUNKSIZE;
    writeBufferChunkSize = QRINGBUFFER_CHUNKSIZE;
    processChannelMode = QProcess::SeparateChannels;
    inputChannelMode = QProcess::ManagedInputChannel;
    processError = QProcess::UnknownError;
    processState = QProcess::NotRunning;
    pid = 0;
    sequenceNumber = 0;
    exitCode = 0;
    exitStatus = QProcess::NormalExit;
    startupSocketNotifier = 0;
    deathNotifier = 0;
    childStartedPipe[0] = INVALID_Q_PIPE;
    childStartedPipe[1] = INVALID_Q_PIPE;
    forkfd = -1;
    crashed = false;
    dying = false;
    emittedReadyRead = false;
    emittedBytesWritten = false;
#ifdef Q_OS_WIN
    stdinWriteTrigger = 0;
    processFinishedNotifier = 0;
#endif // Q_OS_WIN
}

/*!
    \internal
*/
QProcessPrivate::~QProcessPrivate()
{
    if (stdinChannel.process)
        stdinChannel.process->stdoutChannel.clear();
    if (stdoutChannel.process)
        stdoutChannel.process->stdinChannel.clear();
}

/*!
    \internal
*/
void QProcessPrivate::cleanup()
{
    q_func()->setProcessState(QProcess::NotRunning);
#ifdef Q_OS_WIN
    if (pid) {
        CloseHandle(pid->hThread);
        CloseHandle(pid->hProcess);
        delete pid;
        pid = 0;
    }
    if (stdinWriteTrigger) {
        delete stdinWriteTrigger;
        stdinWriteTrigger = 0;
    }
    if (processFinishedNotifier) {
        delete processFinishedNotifier;
        processFinishedNotifier = 0;
    }

#endif
    pid = 0;
    sequenceNumber = 0;
    dying = false;

    if (stdoutChannel.notifier) {
        delete stdoutChannel.notifier;
        stdoutChannel.notifier = 0;
    }
    if (stderrChannel.notifier) {
        delete stderrChannel.notifier;
        stderrChannel.notifier = 0;
    }
    if (stdinChannel.notifier) {
        delete stdinChannel.notifier;
        stdinChannel.notifier = 0;
    }
    if (startupSocketNotifier) {
        delete startupSocketNotifier;
        startupSocketNotifier = 0;
    }
    if (deathNotifier) {
        delete deathNotifier;
        deathNotifier = 0;
    }
    closeChannel(&stdoutChannel);
    closeChannel(&stderrChannel);
    closeChannel(&stdinChannel);
    destroyPipe(childStartedPipe);
#ifdef Q_OS_UNIX
    if (forkfd != -1)
        qt_safe_close(forkfd);
    forkfd = -1;
#endif
}

/*!
    \internal
*/
void QProcessPrivate::setError(QProcess::ProcessError error, const QString &description)
{
    processError = error;
    if (description.isEmpty()) {
        switch (error) {
        case QProcess::FailedToStart:
            errorString = QProcess::tr("Process failed to start");
            break;
        case QProcess::Crashed:
            errorString = QProcess::tr("Process crashed");
            break;
        case QProcess::Timedout:
            errorString = QProcess::tr("Process operation timed out");
            break;
        case QProcess::ReadError:
            errorString = QProcess::tr("Error reading from process");
            break;
        case QProcess::WriteError:
            errorString = QProcess::tr("Error writing to process");
            break;
        case QProcess::UnknownError:
            errorString.clear();
            break;
        }
    } else {
        errorString = description;
    }
}

/*!
    \internal
*/
void QProcessPrivate::setErrorAndEmit(QProcess::ProcessError error, const QString &description)
{
    Q_Q(QProcess);
    Q_ASSERT(error != QProcess::UnknownError);
    setError(error, description);
    emit q->errorOccurred(processError);
    emit q->error(processError);
}

/*!
    \internal
    Returns true if we emitted readyRead().
*/
bool QProcessPrivate::tryReadFromChannel(Channel *channel)
{
    Q_Q(QProcess);
    if (channel->pipe[0] == INVALID_Q_PIPE)
        return false;

    qint64 available = bytesAvailableInChannel(channel);
    if (available == 0)
        available = 1;      // always try to read at least one byte

    QProcess::ProcessChannel channelIdx = (channel == &stdoutChannel
                                           ? QProcess::StandardOutput
                                           : QProcess::StandardError);
    Q_ASSERT(readBuffers.size() > int(channelIdx));
    QRingBuffer &readBuffer = readBuffers[int(channelIdx)];
    char *ptr = readBuffer.reserve(available);
    qint64 readBytes = readFromChannel(channel, ptr, available);
    if (readBytes <= 0)
        readBuffer.chop(available);
    if (readBytes == -2) {
        // EWOULDBLOCK
        return false;
    }
    if (readBytes == -1) {
        setErrorAndEmit(QProcess::ReadError);
#if defined QPROCESS_DEBUG
        qDebug("QProcessPrivate::tryReadFromChannel(%d), failed to read from the process",
               int(channel - &stdinChannel));
#endif
        return false;
    }
    if (readBytes == 0) {
        // EOF
        if (channel->notifier)
            channel->notifier->setEnabled(false);
        closeChannel(channel);
#if defined QPROCESS_DEBUG
        qDebug("QProcessPrivate::tryReadFromChannel(%d), 0 bytes available",
               int(channel - &stdinChannel));
#endif
        return false;
    }
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::tryReadFromChannel(%d), read %d bytes from the process' output",
           int(channel - &stdinChannel), int(readBytes));
#endif

    if (channel->closed) {
        readBuffer.chop(readBytes);
        return false;
    }

    readBuffer.chop(available - readBytes);

    bool didRead = false;
    if (currentReadChannel == channelIdx) {
        didRead = true;
        if (!emittedReadyRead) {
            emittedReadyRead = true;
            emit q->readyRead();
            emittedReadyRead = false;
        }
    }
    emit q->channelReadyRead(int(channelIdx));
    if (channelIdx == QProcess::StandardOutput)
        emit q->readyReadStandardOutput(QProcess::QPrivateSignal());
    else
        emit q->readyReadStandardError(QProcess::QPrivateSignal());
    return didRead;
}

/*!
    \internal
*/
bool QProcessPrivate::_q_canReadStandardOutput()
{
    return tryReadFromChannel(&stdoutChannel);
}

/*!
    \internal
*/
bool QProcessPrivate::_q_canReadStandardError()
{
    return tryReadFromChannel(&stderrChannel);
}

/*!
    \internal
*/
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

/*!
    \internal
*/
bool QProcessPrivate::_q_processDied()
{
    Q_Q(QProcess);
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::_q_processDied()");
#endif
#ifdef Q_OS_UNIX
    if (!waitForDeadChild())
        return false;
#endif
#ifdef Q_OS_WIN
    if (processFinishedNotifier)
        processFinishedNotifier->setEnabled(false);
    drainOutputPipes();
#endif

    // the process may have died before it got a chance to report that it was
    // either running or stopped, so we will call _q_startupNotification() and
    // give it a chance to emit started() or errorOccurred(FailedToStart).
    if (processState == QProcess::Starting) {
        if (!_q_startupNotification())
            return true;
    }

    if (dying) {
        // at this point we know the process is dead. prevent
        // reentering this slot recursively by calling waitForFinished()
        // or opening a dialog inside slots connected to the readyRead
        // signals emitted below.
        return true;
    }
    dying = true;

    // in case there is data in the pipe line and this slot by chance
    // got called before the read notifications, call these two slots
    // so the data is made available before the process dies.
    _q_canReadStandardOutput();
    _q_canReadStandardError();

    findExitCode();

    if (crashed) {
        exitStatus = QProcess::CrashExit;
        setErrorAndEmit(QProcess::Crashed);
    }

    bool wasRunning = (processState == QProcess::Running);

    cleanup();

    if (wasRunning) {
        // we received EOF now:
        emit q->readChannelFinished();
        // in the future:
        //emit q->standardOutputClosed();
        //emit q->standardErrorClosed();

        emit q->finished(exitCode);
        emit q->finished(exitCode, exitStatus);
    }
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::_q_processDied() process is dead");
#endif
    return true;
}

/*!
    \internal
*/
bool QProcessPrivate::_q_startupNotification()
{
    Q_Q(QProcess);
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::startupNotification()");
#endif

    if (startupSocketNotifier)
        startupSocketNotifier->setEnabled(false);
    QString errorMessage;
    if (processStarted(&errorMessage)) {
        q->setProcessState(QProcess::Running);
        emit q->started(QProcess::QPrivateSignal());
        return true;
    }

    q->setProcessState(QProcess::NotRunning);
    setErrorAndEmit(QProcess::FailedToStart, errorMessage);
#ifdef Q_OS_UNIX
    // make sure the process manager removes this entry
    waitForDeadChild();
    findExitCode();
#endif
    cleanup();
    return false;
}

/*!
    \internal
*/
void QProcessPrivate::closeWriteChannel()
{
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::closeWriteChannel()");
#endif
    if (stdinChannel.notifier) {
        delete stdinChannel.notifier;
        stdinChannel.notifier = 0;
    }
#ifdef Q_OS_WIN
    // ### Find a better fix, feeding the process little by little
    // instead.
    flushPipeWriter();
#endif
    closeChannel(&stdinChannel);
}

/*!
    Constructs a QProcess object with the given \a parent.
*/
QProcess::QProcess(QObject *parent)
    : QIODevice(*new QProcessPrivate, parent)
{
#if defined QPROCESS_DEBUG
    qDebug("QProcess::QProcess(%p)", parent);
#endif
}

/*!
    Destructs the QProcess object, i.e., killing the process.

    Note that this function will not return until the process is
    terminated.
*/
QProcess::~QProcess()
{
    Q_D(QProcess);
    if (d->processState != NotRunning) {
        qWarning().nospace()
            << "QProcess: Destroyed while process (" << QDir::toNativeSeparators(program()) << ") is still running.";
        kill();
        waitForFinished();
    }
#ifdef Q_OS_UNIX
    // make sure the process manager removes this entry
    d->findExitCode();
#endif
    d->cleanup();
}

/*!
    \obsolete
    Returns the read channel mode of the QProcess. This function is
    equivalent to processChannelMode()

    \sa processChannelMode()
*/
QProcess::ProcessChannelMode QProcess::readChannelMode() const
{
    return processChannelMode();
}

/*!
    \obsolete

    Use setProcessChannelMode(\a mode) instead.

    \sa setProcessChannelMode()
*/
void QProcess::setReadChannelMode(ProcessChannelMode mode)
{
    setProcessChannelMode(mode);
}

/*!
    \since 4.2

    Returns the channel mode of the QProcess standard output and
    standard error channels.

    \sa setProcessChannelMode(), ProcessChannelMode, setReadChannel()
*/
QProcess::ProcessChannelMode QProcess::processChannelMode() const
{
    Q_D(const QProcess);
    return d->processChannelMode;
}

/*!
    \since 4.2

    Sets the channel mode of the QProcess standard output and standard
    error channels to the \a mode specified.
    This mode will be used the next time start() is called. For example:

    \snippet code/src_corelib_io_qprocess.cpp 0

    \sa processChannelMode(), ProcessChannelMode, setReadChannel()
*/
void QProcess::setProcessChannelMode(ProcessChannelMode mode)
{
    Q_D(QProcess);
    d->processChannelMode = mode;
}

/*!
    \since 5.2

    Returns the channel mode of the QProcess standard input channel.

    \sa setInputChannelMode(), InputChannelMode
*/
QProcess::InputChannelMode QProcess::inputChannelMode() const
{
    Q_D(const QProcess);
    return d->inputChannelMode;
}

/*!
    \since 5.2

    Sets the channel mode of the QProcess standard input
    channel to the \a mode specified.
    This mode will be used the next time start() is called.

    \sa inputChannelMode(), InputChannelMode
*/
void QProcess::setInputChannelMode(InputChannelMode mode)
{
    Q_D(QProcess);
    d->inputChannelMode = mode;
}

/*!
    Returns the current read channel of the QProcess.

    \sa setReadChannel()
*/
QProcess::ProcessChannel QProcess::readChannel() const
{
    Q_D(const QProcess);
    return ProcessChannel(d->currentReadChannel);
}

/*!
    Sets the current read channel of the QProcess to the given \a
    channel. The current input channel is used by the functions
    read(), readAll(), readLine(), and getChar(). It also determines
    which channel triggers QProcess to emit readyRead().

    \sa readChannel()
*/
void QProcess::setReadChannel(ProcessChannel channel)
{
    QIODevice::setCurrentReadChannel(int(channel));
}

/*!
    Closes the read channel \a channel. After calling this function,
    QProcess will no longer receive data on the channel. Any data that
    has already been received is still available for reading.

    Call this function to save memory, if you are not interested in
    the output of the process.

    \sa closeWriteChannel(), setReadChannel()
*/
void QProcess::closeReadChannel(ProcessChannel channel)
{
    Q_D(QProcess);

    if (channel == StandardOutput)
        d->stdoutChannel.closed = true;
    else
        d->stderrChannel.closed = true;
}

/*!
    Schedules the write channel of QProcess to be closed. The channel
    will close once all data has been written to the process. After
    calling this function, any attempts to write to the process will
    fail.

    Closing the write channel is necessary for programs that read
    input data until the channel has been closed. For example, the
    program "more" is used to display text data in a console on both
    Unix and Windows. But it will not display the text data until
    QProcess's write channel has been closed. Example:

    \snippet code/src_corelib_io_qprocess.cpp 1

    The write channel is implicitly opened when start() is called.

    \sa closeReadChannel()
*/
void QProcess::closeWriteChannel()
{
    Q_D(QProcess);
    d->stdinChannel.closed = true; // closing
    if (d->writeBuffer.isEmpty())
        d->closeWriteChannel();
}

/*!
    \since 4.2

    Redirects the process' standard input to the file indicated by \a
    fileName. When an input redirection is in place, the QProcess
    object will be in read-only mode (calling write() will result in
    error).

    To make the process read EOF right away, pass nullDevice() here.
    This is cleaner than using closeWriteChannel() before writing any
    data, because it can be set up prior to starting the process.

    If the file \a fileName does not exist at the moment start() is
    called or is not readable, starting the process will fail.

    Calling setStandardInputFile() after the process has started has no
    effect.

    \sa setStandardOutputFile(), setStandardErrorFile(),
        setStandardOutputProcess()
*/
void QProcess::setStandardInputFile(const QString &fileName)
{
    Q_D(QProcess);
    d->stdinChannel = fileName;
}

/*!
    \since 4.2

    Redirects the process' standard output to the file \a
    fileName. When the redirection is in place, the standard output
    read channel is closed: reading from it using read() will always
    fail, as will readAllStandardOutput().

    To discard all standard output from the process, pass nullDevice()
    here. This is more efficient than simply never reading the standard
    output, as no QProcess buffers are filled.

    If the file \a fileName doesn't exist at the moment start() is
    called, it will be created. If it cannot be created, the starting
    will fail.

    If the file exists and \a mode is QIODevice::Truncate, the file
    will be truncated. Otherwise (if \a mode is QIODevice::Append),
    the file will be appended to.

    Calling setStandardOutputFile() after the process has started has
    no effect.

    \sa setStandardInputFile(), setStandardErrorFile(),
        setStandardOutputProcess()
*/
void QProcess::setStandardOutputFile(const QString &fileName, OpenMode mode)
{
    Q_ASSERT(mode == Append || mode == Truncate);
    Q_D(QProcess);

    d->stdoutChannel = fileName;
    d->stdoutChannel.append = mode == Append;
}

/*!
    \since 4.2

    Redirects the process' standard error to the file \a
    fileName. When the redirection is in place, the standard error
    read channel is closed: reading from it using read() will always
    fail, as will readAllStandardError(). The file will be appended to
    if \a mode is Append, otherwise, it will be truncated.

    See setStandardOutputFile() for more information on how the file
    is opened.

    Note: if setProcessChannelMode() was called with an argument of
    QProcess::MergedChannels, this function has no effect.

    \sa setStandardInputFile(), setStandardOutputFile(),
        setStandardOutputProcess()
*/
void QProcess::setStandardErrorFile(const QString &fileName, OpenMode mode)
{
    Q_ASSERT(mode == Append || mode == Truncate);
    Q_D(QProcess);

    d->stderrChannel = fileName;
    d->stderrChannel.append = mode == Append;
}

/*!
    \since 4.2

    Pipes the standard output stream of this process to the \a
    destination process' standard input.

    The following shell command:
    \snippet code/src_corelib_io_qprocess.cpp 2

    Can be accomplished with QProcess with the following code:
    \snippet code/src_corelib_io_qprocess.cpp 3
*/
void QProcess::setStandardOutputProcess(QProcess *destination)
{
    QProcessPrivate *dfrom = d_func();
    QProcessPrivate *dto = destination->d_func();
    dfrom->stdoutChannel.pipeTo(dto);
    dto->stdinChannel.pipeFrom(dfrom);
}

#if defined(Q_OS_WIN) || defined(Q_CLANG_QDOC)

/*!
    \since 4.7

    Returns the additional native command line arguments for the program.

    \note This function is available only on the Windows platform.

    \sa setNativeArguments()
*/
QString QProcess::nativeArguments() const
{
    Q_D(const QProcess);
    return d->nativeArguments;
}

/*!
    \since 4.7
    \overload

    Sets additional native command line \a arguments for the program.

    On operating systems where the system API for passing command line
    \a arguments to a subprocess natively uses a single string, one can
    conceive command lines which cannot be passed via QProcess's portable
    list-based API. In such cases this function must be used to set a
    string which is \e appended to the string composed from the usual
    argument list, with a delimiting space.

    \note This function is available only on the Windows platform.

    \sa nativeArguments()
*/
void QProcess::setNativeArguments(const QString &arguments)
{
    Q_D(QProcess);
    d->nativeArguments = arguments;
}

/*!
    \since 5.7

    Returns a previously set \c CreateProcess modifier function.

    \note This function is available only on the Windows platform.

    \sa setCreateProcessArgumentsModifier()
    \sa QProcess::CreateProcessArgumentModifier
*/
QProcess::CreateProcessArgumentModifier QProcess::createProcessArgumentsModifier() const
{
    Q_D(const QProcess);
    return d->modifyCreateProcessArgs;
}

/*!
    \since 5.7

    Sets the \a modifier for the \c CreateProcess Win32 API call.
    Pass \c QProcess::CreateProcessArgumentModifier() to remove a previously set one.

    \note This function is available only on the Windows platform and requires
    C++11.

    \sa QProcess::CreateProcessArgumentModifier
*/
void QProcess::setCreateProcessArgumentsModifier(CreateProcessArgumentModifier modifier)
{
    Q_D(QProcess);
    d->modifyCreateProcessArgs = modifier;
}

#endif

/*!
    If QProcess has been assigned a working directory, this function returns
    the working directory that the QProcess will enter before the program has
    started. Otherwise, (i.e., no directory has been assigned,) an empty
    string is returned, and QProcess will use the application's current
    working directory instead.

    \sa setWorkingDirectory()
*/
QString QProcess::workingDirectory() const
{
    Q_D(const QProcess);
    return d->workingDirectory;
}

/*!
    Sets the working directory to \a dir. QProcess will start the
    process in this directory. The default behavior is to start the
    process in the working directory of the calling process.

    \note On QNX, this may cause all application threads to
    temporarily freeze.

    \sa workingDirectory(), start()
*/
void QProcess::setWorkingDirectory(const QString &dir)
{
    Q_D(QProcess);
    d->workingDirectory = dir;
}


/*!
    \deprecated
    Use processId() instead.

    Returns the native process identifier for the running process, if
    available.  If no process is currently running, \c 0 is returned.

    \note Unlike \l processId(), pid() returns an integer on Unix and a pointer on Windows.

    \sa Q_PID, processId()
*/
Q_PID QProcess::pid() const // ### Qt 6 remove or rename this method to processInformation()
{
    Q_D(const QProcess);
    return d->pid;
}

/*!
    \since 5.3

    Returns the native process identifier for the running process, if
    available. If no process is currently running, \c 0 is returned.
 */
qint64 QProcess::processId() const
{
    Q_D(const QProcess);
#ifdef Q_OS_WIN
    return d->pid ? d->pid->dwProcessId : 0;
#else
    return d->pid;
#endif
}

/*! \reimp

    This function operates on the current read channel.

    \sa readChannel(), setReadChannel()
*/
bool QProcess::canReadLine() const
{
    return QIODevice::canReadLine();
}

/*!
    Closes all communication with the process and kills it. After calling this
    function, QProcess will no longer emit readyRead(), and data can no
    longer be read or written.
*/
void QProcess::close()
{
    Q_D(QProcess);
    emit aboutToClose();
    while (waitForBytesWritten(-1))
        ;
    kill();
    waitForFinished(-1);
    d->setWriteChannelCount(0);
    QIODevice::close();
}

/*! \reimp

   Returns \c true if the process is not running, and no more data is available
   for reading; otherwise returns \c false.
*/
bool QProcess::atEnd() const
{
    return QIODevice::atEnd();
}

/*! \reimp
*/
bool QProcess::isSequential() const
{
    return true;
}

/*! \reimp
*/
qint64 QProcess::bytesAvailable() const
{
    return QIODevice::bytesAvailable();
}

/*! \reimp
*/
qint64 QProcess::bytesToWrite() const
{
    qint64 size = QIODevice::bytesToWrite();
#ifdef Q_OS_WIN
    size += d_func()->pipeWriterBytesToWrite();
#endif
    return size;
}

/*!
    Returns the type of error that occurred last.

    \sa state()
*/
QProcess::ProcessError QProcess::error() const
{
    Q_D(const QProcess);
    return d->processError;
}

/*!
    Returns the current state of the process.

    \sa stateChanged(), error()
*/
QProcess::ProcessState QProcess::state() const
{
    Q_D(const QProcess);
    return d->processState;
}

/*!
    \deprecated
    Sets the environment that QProcess will pass to the child process.
    The parameter \a environment is a list of key=value pairs.

    For example, the following code adds the environment variable \c{TMPDIR}:

    \snippet qprocess-environment/main.cpp 0

    \note This function is less efficient than the setProcessEnvironment()
    function.

    \sa environment(), setProcessEnvironment(), systemEnvironment()
*/
void QProcess::setEnvironment(const QStringList &environment)
{
    setProcessEnvironment(QProcessEnvironmentPrivate::fromList(environment));
}

/*!
    \deprecated
    Returns the environment that QProcess will pass to its child
    process, or an empty QStringList if no environment has been set
    using setEnvironment(). If no environment has been set, the
    environment of the calling process will be used.

    \sa processEnvironment(), setEnvironment(), systemEnvironment()
*/
QStringList QProcess::environment() const
{
    Q_D(const QProcess);
    return d->environment.toStringList();
}

/*!
    \since 4.6
    Sets the \a environment that QProcess will pass to the child process.

    For example, the following code adds the environment variable \c{TMPDIR}:

    \snippet qprocess-environment/main.cpp 1

    Note how, on Windows, environment variable names are case-insensitive.

    \sa processEnvironment(), QProcessEnvironment::systemEnvironment(), setEnvironment()
*/
void QProcess::setProcessEnvironment(const QProcessEnvironment &environment)
{
    Q_D(QProcess);
    d->environment = environment;
}

/*!
    \since 4.6
    Returns the environment that QProcess will pass to its child
    process, or an empty object if no environment has been set using
    setEnvironment() or setProcessEnvironment(). If no environment has
    been set, the environment of the calling process will be used.

    \sa setProcessEnvironment(), setEnvironment(), QProcessEnvironment::isEmpty()
*/
QProcessEnvironment QProcess::processEnvironment() const
{
    Q_D(const QProcess);
    return d->environment;
}

/*!
    Blocks until the process has started and the started() signal has
    been emitted, or until \a msecs milliseconds have passed.

    Returns \c true if the process was started successfully; otherwise
    returns \c false (if the operation timed out or if an error
    occurred).

    This function can operate without an event loop. It is
    useful when writing non-GUI applications and when performing
    I/O operations in a non-GUI thread.

    \warning Calling this function from the main (GUI) thread
    might cause your user interface to freeze.

    If msecs is -1, this function will not time out.

    \note On some UNIX operating systems, this function may return true but
    the process may later report a QProcess::FailedToStart error.

    \sa started(), waitForReadyRead(), waitForBytesWritten(), waitForFinished()
*/
bool QProcess::waitForStarted(int msecs)
{
    Q_D(QProcess);
    if (d->processState == QProcess::Starting)
        return d->waitForStarted(msecs);

    return d->processState == QProcess::Running;
}

/*! \reimp
*/
bool QProcess::waitForReadyRead(int msecs)
{
    Q_D(QProcess);

    if (d->processState == QProcess::NotRunning)
        return false;
    if (d->currentReadChannel == QProcess::StandardOutput && d->stdoutChannel.closed)
        return false;
    if (d->currentReadChannel == QProcess::StandardError && d->stderrChannel.closed)
        return false;
    return d->waitForReadyRead(msecs);
}

/*! \reimp
*/
bool QProcess::waitForBytesWritten(int msecs)
{
    Q_D(QProcess);
    if (d->processState == QProcess::NotRunning)
        return false;
    if (d->processState == QProcess::Starting) {
        QElapsedTimer stopWatch;
        stopWatch.start();
        bool started = waitForStarted(msecs);
        if (!started)
            return false;
        msecs = qt_subtract_from_timeout(msecs, stopWatch.elapsed());
    }

    return d->waitForBytesWritten(msecs);
}

/*!
    Blocks until the process has finished and the finished() signal
    has been emitted, or until \a msecs milliseconds have passed.

    Returns \c true if the process finished; otherwise returns \c false (if
    the operation timed out, if an error occurred, or if this QProcess
    is already finished).

    This function can operate without an event loop. It is
    useful when writing non-GUI applications and when performing
    I/O operations in a non-GUI thread.

    \warning Calling this function from the main (GUI) thread
    might cause your user interface to freeze.

    If msecs is -1, this function will not time out.

    \sa finished(), waitForStarted(), waitForReadyRead(), waitForBytesWritten()
*/
bool QProcess::waitForFinished(int msecs)
{
    Q_D(QProcess);
    if (d->processState == QProcess::NotRunning)
        return false;
    if (d->processState == QProcess::Starting) {
        QElapsedTimer stopWatch;
        stopWatch.start();
        bool started = waitForStarted(msecs);
        if (!started)
            return false;
        msecs = qt_subtract_from_timeout(msecs, stopWatch.elapsed());
    }

    return d->waitForFinished(msecs);
}

/*!
    Sets the current state of the QProcess to the \a state specified.

    \sa state()
*/
void QProcess::setProcessState(ProcessState state)
{
    Q_D(QProcess);
    if (d->processState == state)
        return;
    d->processState = state;
    emit stateChanged(state, QPrivateSignal());
}

/*!
  This function is called in the child process context just before the
    program is executed on Unix or \macos (i.e., after \c fork(), but before
    \c execve()). Reimplement this function to do last minute initialization
    of the child process. Example:

    \snippet code/src_corelib_io_qprocess.cpp 4

    You cannot exit the process (by calling exit(), for instance) from
    this function. If you need to stop the program before it starts
    execution, your workaround is to emit finished() and then call
    exit().

    \warning This function is called by QProcess on Unix and \macos
    only. On Windows and QNX, it is not called.
*/
void QProcess::setupChildProcess()
{
}

/*! \reimp
*/
qint64 QProcess::readData(char *data, qint64 maxlen)
{
    Q_D(QProcess);
    Q_UNUSED(data);
    if (!maxlen)
        return 0;
    if (d->processState == QProcess::NotRunning)
        return -1;              // EOF
    return 0;
}

/*! \reimp
*/
qint64 QProcess::writeData(const char *data, qint64 len)
{
    Q_D(QProcess);

    if (d->stdinChannel.closed) {
#if defined QPROCESS_DEBUG
    qDebug("QProcess::writeData(%p \"%s\", %lld) == 0 (write channel closing)",
           data, qt_prettyDebug(data, len, 16).constData(), len);
#endif
        return 0;
    }

#if defined(Q_OS_WIN)
    if (!d->stdinWriteTrigger) {
        d->stdinWriteTrigger = new QTimer;
        d->stdinWriteTrigger->setSingleShot(true);
        QObjectPrivate::connect(d->stdinWriteTrigger, &QTimer::timeout,
                                d, &QProcessPrivate::_q_canWrite);
    }
#endif

    d->writeBuffer.append(data, len);
#ifdef Q_OS_WIN
    if (!d->stdinWriteTrigger->isActive())
        d->stdinWriteTrigger->start();
#else
    if (d->stdinChannel.notifier)
        d->stdinChannel.notifier->setEnabled(true);
#endif
#if defined QPROCESS_DEBUG
    qDebug("QProcess::writeData(%p \"%s\", %lld) == %lld (written to buffer)",
           data, qt_prettyDebug(data, len, 16).constData(), len, len);
#endif
    return len;
}

/*!
    Regardless of the current read channel, this function returns all
    data available from the standard output of the process as a
    QByteArray.

    \sa readyReadStandardOutput(), readAllStandardError(), readChannel(), setReadChannel()
*/
QByteArray QProcess::readAllStandardOutput()
{
    ProcessChannel tmp = readChannel();
    setReadChannel(StandardOutput);
    QByteArray data = readAll();
    setReadChannel(tmp);
    return data;
}

/*!
    Regardless of the current read channel, this function returns all
    data available from the standard error of the process as a
    QByteArray.

    \sa readyReadStandardError(), readAllStandardOutput(), readChannel(), setReadChannel()
*/
QByteArray QProcess::readAllStandardError()
{
    ProcessChannel tmp = readChannel();
    setReadChannel(StandardError);
    QByteArray data = readAll();
    setReadChannel(tmp);
    return data;
}

/*!
    Starts the given \a program in a new process, passing the command line
    arguments in \a arguments.

    The QProcess object will immediately enter the Starting state. If the
    process starts successfully, QProcess will emit started(); otherwise,
    errorOccurred() will be emitted.

    \note Processes are started asynchronously, which means the started()
    and errorOccurred() signals may be delayed. Call waitForStarted() to make
    sure the process has started (or has failed to start) and those signals
    have been emitted.

    \note No further splitting of the arguments is performed.

    \b{Windows:} The arguments are quoted and joined into a command line
    that is compatible with the \c CommandLineToArgvW() Windows function.
    For programs that have different command line quoting requirements,
    you need to use setNativeArguments(). One notable program that does
    not follow the \c CommandLineToArgvW() rules is cmd.exe and, by
    consequence, all batch scripts.

    The OpenMode is set to \a mode.

    If the QProcess object is already running a process, a warning may be
    printed at the console, and the existing process will continue running
    unaffected.

    \sa processId(), started(), waitForStarted(), setNativeArguments()
*/
void QProcess::start(const QString &program, const QStringList &arguments, OpenMode mode)
{
    Q_D(QProcess);
    if (d->processState != NotRunning) {
        qWarning("QProcess::start: Process is already running");
        return;
    }
    if (program.isEmpty()) {
        d->setErrorAndEmit(QProcess::FailedToStart, tr("No program defined"));
        return;
    }

    d->program = program;
    d->arguments = arguments;

    d->start(mode);
}

/*!
    \since 5.1
    \overload

    Starts the program set by setProgram() with arguments set by setArguments().
    The OpenMode is set to \a mode.

    \sa open(), setProgram(), setArguments()
 */
void QProcess::start(OpenMode mode)
{
    Q_D(QProcess);
    if (d->processState != NotRunning) {
        qWarning("QProcess::start: Process is already running");
        return;
    }
    if (d->program.isEmpty()) {
        d->setErrorAndEmit(QProcess::FailedToStart, tr("No program defined"));
        return;
    }

    d->start(mode);
}

/*!
    \since 5.10

    Starts the program set by setProgram() with arguments set by setArguments()
    in a new process, and detaches from it. Returns \c true on success;
    otherwise returns \c false. If the calling process exits, the
    detached process will continue to run unaffected.

    \b{Unix:} The started process will run in its own session and act
    like a daemon.

    The process will be started in the directory set by setWorkingDirectory().
    If workingDirectory() is empty, the working directory is inherited
    from the calling process.

    \note On QNX, this may cause all application threads to
    temporarily freeze.

    If the function is successful then *\a pid is set to the process identifier
    of the started process. Note that the child process may exit and the PID
    may become invalid without notice. Furthermore, after the child process
    exits, the same PID may be recycled and used by a completely different
    process. User code should be careful when using this variable, especially
    if one intends to forcibly terminate the process by operating system means.

    Only the following property setters are supported by startDetached():
    \list
    \li setArguments()
    \li setCreateProcessArgumentsModifier()
    \li setNativeArguments()
    \li setProcessEnvironment()
    \li setProgram()
    \li setStandardErrorFile()
    \li setStandardInputFile()
    \li setStandardOutputFile()
    \li setWorkingDirectory()
    \endlist
    All other properties of the QProcess object are ignored.

    \note The called process inherits the console window of the calling
    process. To suppress console output, redirect standard/error output to
    QProcess::nullDevice().

    \sa start()
    \sa startDetached(const QString &program, const QStringList &arguments,
                      const QString &workingDirectory, qint64 *pid)
    \sa startDetached(const QString &command)
*/
bool QProcess::startDetached(qint64 *pid)
{
    Q_D(QProcess);
    if (d->processState != NotRunning) {
        qWarning("QProcess::startDetached: Process is already running");
        return false;
    }
    if (d->program.isEmpty()) {
        d->setErrorAndEmit(QProcess::FailedToStart, tr("No program defined"));
        return false;
    }
    return d->startDetached(pid);
}

/*!
    Starts the program set by setProgram() with arguments set by setArguments().
    The OpenMode is set to \a mode.

    This method is an alias for start(), and exists only to fully implement
    the interface defined by QIODevice.

    \sa start(), setProgram(), setArguments()
*/
bool QProcess::open(OpenMode mode)
{
    Q_D(QProcess);
    if (d->processState != NotRunning) {
        qWarning("QProcess::start: Process is already running");
        return false;
    }
    if (d->program.isEmpty()) {
        qWarning("QProcess::start: program not set");
        return false;
    }

    d->start(mode);
    return true;
}

void QProcessPrivate::start(QIODevice::OpenMode mode)
{
    Q_Q(QProcess);
#if defined QPROCESS_DEBUG
    qDebug() << "QProcess::start(" << program << ',' << arguments << ',' << mode << ')';
#endif

    if (stdinChannel.type != QProcessPrivate::Channel::Normal)
        mode &= ~QIODevice::WriteOnly;     // not open for writing
    if (stdoutChannel.type != QProcessPrivate::Channel::Normal &&
        (stderrChannel.type != QProcessPrivate::Channel::Normal ||
         processChannelMode == QProcess::MergedChannels))
        mode &= ~QIODevice::ReadOnly;      // not open for reading
    if (mode == 0)
        mode = QIODevice::Unbuffered;
    if ((mode & QIODevice::ReadOnly) == 0) {
        if (stdoutChannel.type == QProcessPrivate::Channel::Normal)
            q->setStandardOutputFile(q->nullDevice());
        if (stderrChannel.type == QProcessPrivate::Channel::Normal
            && processChannelMode != QProcess::MergedChannels)
            q->setStandardErrorFile(q->nullDevice());
    }

    q->QIODevice::open(mode);

    if (q->isReadable() && processChannelMode != QProcess::MergedChannels)
        setReadChannelCount(2);

    stdinChannel.closed = false;
    stdoutChannel.closed = false;
    stderrChannel.closed = false;

    exitCode = 0;
    exitStatus = QProcess::NormalExit;
    processError = QProcess::UnknownError;
    errorString.clear();
    startProcess();
}


static QStringList parseCombinedArgString(const QString &program)
{
    QStringList args;
    QString tmp;
    int quoteCount = 0;
    bool inQuote = false;

    // handle quoting. tokens can be surrounded by double quotes
    // "hello world". three consecutive double quotes represent
    // the quote character itself.
    for (int i = 0; i < program.size(); ++i) {
        if (program.at(i) == QLatin1Char('"')) {
            ++quoteCount;
            if (quoteCount == 3) {
                // third consecutive quote
                quoteCount = 0;
                tmp += program.at(i);
            }
            continue;
        }
        if (quoteCount) {
            if (quoteCount == 1)
                inQuote = !inQuote;
            quoteCount = 0;
        }
        if (!inQuote && program.at(i).isSpace()) {
            if (!tmp.isEmpty()) {
                args += tmp;
                tmp.clear();
            }
        } else {
            tmp += program.at(i);
        }
    }
    if (!tmp.isEmpty())
        args += tmp;

    return args;
}

/*!
    \overload

    Starts the command \a command in a new process.
    The OpenMode is set to \a mode.

    \a command is a single string of text containing both the program name
    and its arguments. The arguments are separated by one or more spaces.
    For example:

    \snippet code/src_corelib_io_qprocess.cpp 5

    Arguments containing spaces must be quoted to be correctly supplied to
    the new process. For example:

    \snippet code/src_corelib_io_qprocess.cpp 6

    Literal quotes in the \a command string are represented by triple quotes.
    For example:

    \snippet code/src_corelib_io_qprocess.cpp 7

    After the \a command string has been split and unquoted, this function
    behaves like the overload which takes the arguments as a string list.

    You can disable this overload by defining \c
    QT_NO_PROCESS_COMBINED_ARGUMENT_START when you compile your applications.
    This can be useful if you want to ensure that you are not splitting arguments
    unintentionally, for example. In virtually all cases, using the other overload
    is the preferred method.

    On operating systems where the system API for passing command line
    arguments to a subprocess natively uses a single string (Windows), one can
    conceive command lines which cannot be passed via QProcess's portable
    list-based API. In these rare cases you need to use setProgram() and
    setNativeArguments() instead of this function.

*/
#if !defined(QT_NO_PROCESS_COMBINED_ARGUMENT_START)
void QProcess::start(const QString &command, OpenMode mode)
{
    QStringList args = parseCombinedArgString(command);
    if (args.isEmpty()) {
        Q_D(QProcess);
        d->setErrorAndEmit(QProcess::FailedToStart, tr("No program defined"));
        return;
    }

    const QString prog = args.takeFirst();

    start(prog, args, mode);
}
#endif

/*!
    \since 5.0

    Returns the program the process was last started with.

    \sa start()
*/
QString QProcess::program() const
{
    Q_D(const QProcess);
    return d->program;
}

/*!
    \since 5.1

    Set the \a program to use when starting the process.
    This function must be called before start().

    \sa start(), setArguments(), program()
*/
void QProcess::setProgram(const QString &program)
{
    Q_D(QProcess);
    if (d->processState != NotRunning) {
        qWarning("QProcess::setProgram: Process is already running");
        return;
    }
    d->program = program;
}

/*!
    \since 5.0

    Returns the command line arguments the process was last started with.

    \sa start()
*/
QStringList QProcess::arguments() const
{
    Q_D(const QProcess);
    return d->arguments;
}

/*!
    \since 5.1

    Set the \a arguments to pass to the called program when starting the process.
    This function must be called before start().

    \sa start(), setProgram(), arguments()
*/
void QProcess::setArguments(const QStringList &arguments)
{
    Q_D(QProcess);
    if (d->processState != NotRunning) {
        qWarning("QProcess::setProgram: Process is already running");
        return;
    }
    d->arguments = arguments;
}

/*!
    Attempts to terminate the process.

    The process may not exit as a result of calling this function (it is given
    the chance to prompt the user for any unsaved files, etc).

    On Windows, terminate() posts a WM_CLOSE message to all top-level windows
    of the process and then to the main thread of the process itself. On Unix
    and \macos the \c SIGTERM signal is sent.

    Console applications on Windows that do not run an event loop, or whose
    event loop does not handle the WM_CLOSE message, can only be terminated by
    calling kill().

    \sa kill()
*/
void QProcess::terminate()
{
    Q_D(QProcess);
    d->terminateProcess();
}

/*!
    Kills the current process, causing it to exit immediately.

    On Windows, kill() uses TerminateProcess, and on Unix and \macos, the
    SIGKILL signal is sent to the process.

    \sa terminate()
*/
void QProcess::kill()
{
    Q_D(QProcess);
    d->killProcess();
}

/*!
    Returns the exit code of the last process that finished.

    This value is not valid unless exitStatus() returns NormalExit.
*/
int QProcess::exitCode() const
{
    Q_D(const QProcess);
    return d->exitCode;
}

/*!
    \since 4.1

    Returns the exit status of the last process that finished.

    On Windows, if the process was terminated with TerminateProcess() from
    another application, this function will still return NormalExit
    unless the exit code is less than 0.
*/
QProcess::ExitStatus QProcess::exitStatus() const
{
    Q_D(const QProcess);
    return d->exitStatus;
}

/*!
    Starts the program \a program with the arguments \a arguments in a
    new process, waits for it to finish, and then returns the exit
    code of the process. Any data the new process writes to the
    console is forwarded to the calling process.

    The environment and working directory are inherited from the calling
    process.

    Argument handling is identical to the respective start() overload.

    If the process cannot be started, -2 is returned. If the process
    crashes, -1 is returned. Otherwise, the process' exit code is
    returned.

    \sa start()
*/
int QProcess::execute(const QString &program, const QStringList &arguments)
{
    QProcess process;
    process.setReadChannelMode(ForwardedChannels);
    process.start(program, arguments);
    if (!process.waitForFinished(-1) || process.error() == FailedToStart)
        return -2;
    return process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
}

/*!
    \overload

    Starts the program \a command in a new process, waits for it to finish,
    and then returns the exit code.

    Argument handling is identical to the respective start() overload.

    After the \a command string has been split and unquoted, this function
    behaves like the overload which takes the arguments as a string list.

    \sa start()
*/
int QProcess::execute(const QString &command)
{
    QProcess process;
    process.setReadChannelMode(ForwardedChannels);
    process.start(command);
    if (!process.waitForFinished(-1) || process.error() == FailedToStart)
        return -2;
    return process.exitStatus() == QProcess::NormalExit ? process.exitCode() : -1;
}

/*!
    \overload startDetached()

    Starts the program \a program with the arguments \a arguments in a
    new process, and detaches from it. Returns \c true on success;
    otherwise returns \c false. If the calling process exits, the
    detached process will continue to run unaffected.

    Argument handling is identical to the respective start() overload.

    The process will be started in the directory \a workingDirectory.
    If \a workingDirectory is empty, the working directory is inherited
    from the calling process.

    If the function is successful then *\a pid is set to the process
    identifier of the started process.

    \sa start()
*/
bool QProcess::startDetached(const QString &program,
                             const QStringList &arguments,
                             const QString &workingDirectory,
                             qint64 *pid)
{
    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    process.setWorkingDirectory(workingDirectory);
    return process.startDetached(pid);
}

/*!
    \internal
*/
bool QProcess::startDetached(const QString &program,
                             const QStringList &arguments)
{
    QProcess process;
    process.setProgram(program);
    process.setArguments(arguments);
    return process.startDetached();
}

/*!
    \overload startDetached()

    Starts the command \a command in a new process, and detaches from it.
    Returns \c true on success; otherwise returns \c false.

    Argument handling is identical to the respective start() overload.

    After the \a command string has been split and unquoted, this function
    behaves like the overload which takes the arguments as a string list.

    \sa start(const QString &command, OpenMode mode)
*/
bool QProcess::startDetached(const QString &command)
{
    QStringList args = parseCombinedArgString(command);
    if (args.isEmpty())
        return false;

    QProcess process;
    process.setProgram(args.takeFirst());
    process.setArguments(args);
    return process.startDetached();
}

QT_BEGIN_INCLUDE_NAMESPACE
#if defined(Q_OS_MACX)
# include <crt_externs.h>
# define environ (*_NSGetEnviron())
#elif defined(QT_PLATFORM_UIKIT)
  static char *qt_empty_environ[] = { 0 };
#define environ qt_empty_environ
#elif !defined(Q_OS_WIN)
  extern char **environ;
#endif
QT_END_INCLUDE_NAMESPACE

/*!
    \since 4.1

    Returns the environment of the calling process as a list of
    key=value pairs. Example:

    \snippet code/src_corelib_io_qprocess.cpp 8

    This function does not cache the system environment. Therefore, it's
    possible to obtain an updated version of the environment if low-level C
    library functions like \tt setenv or \tt putenv have been called.

    However, note that repeated calls to this function will recreate the
    list of environment variables, which is a non-trivial operation.

    \note For new code, it is recommended to use QProcessEnvironment::systemEnvironment()

    \sa QProcessEnvironment::systemEnvironment(), setProcessEnvironment()
*/
QStringList QProcess::systemEnvironment()
{
    QStringList tmp;
    char *entry = 0;
    int count = 0;
    while ((entry = environ[count++]))
        tmp << QString::fromLocal8Bit(entry);
    return tmp;
}

/*!
    \fn QProcessEnvironment QProcessEnvironment::systemEnvironment()

    \since 4.6

    \brief The systemEnvironment function returns the environment of
    the calling process.

    It is returned as a QProcessEnvironment. This function does not
    cache the system environment. Therefore, it's possible to obtain
    an updated version of the environment if low-level C library
    functions like \tt setenv or \tt putenv have been called.

    However, note that repeated calls to this function will recreate the
    QProcessEnvironment object, which is a non-trivial operation.

    \sa QProcess::systemEnvironment()
*/

/*!
    \since 5.2

    \brief The null device of the operating system.

    The returned file path uses native directory separators.

    \sa QProcess::setStandardInputFile(), QProcess::setStandardOutputFile(),
        QProcess::setStandardErrorFile()
*/
QString QProcess::nullDevice()
{
#ifdef Q_OS_WIN
    return QStringLiteral("\\\\.\\NUL");
#elif defined(_PATH_DEVNULL)
    return QStringLiteral(_PATH_DEVNULL);
#else
    return QStringLiteral("/dev/null");
#endif
}

/*!
    \typedef Q_PID
    \relates QProcess

    Typedef for the identifiers used to represent processes on the underlying
    platform. On Unix, this corresponds to \l qint64; on Windows, it
    corresponds to \c{_PROCESS_INFORMATION*}.

    \sa QProcess::pid()
*/

#endif // QT_CONFIG(process)

QT_END_NAMESPACE

#include "moc_qprocess.cpp"
