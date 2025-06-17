// Copyright (C) 2021 The Qt Company Ltd.
// Copyright (C) 2022 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only
// Qt-Security score:critical reason:execute-external-code

//#define QPROCESS_DEBUG

#include <qdebug.h>
#include <qdir.h>
#include <qscopedvaluerollback.h>

#include "qprocess.h"
#include "qprocess_p.h"

#include <qbytearray.h>
#include <qdeadlinetimer.h>
#include <qcoreapplication.h>

#if __has_include(<paths.h>)
#include <paths.h>
#endif

QT_BEGIN_NAMESPACE

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

    \compares equality

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
    QString::toLocal8Bit).

    On Windows, the variable names are case-insensitive, but case-preserving.
    QProcessEnvironment behaves accordingly.

    \sa QProcess, QProcess::systemEnvironment(), QProcess::setProcessEnvironment()
*/

QStringList QProcessEnvironmentPrivate::toList() const
{
    QStringList result;
    result.reserve(vars.size());
    for (auto it = vars.cbegin(), end = vars.cend(); it != end; ++it)
        result << nameToString(it.key()) + u'=' + valueToString(it.value());
    return result;
}

QProcessEnvironment QProcessEnvironmentPrivate::fromList(const QStringList &list)
{
    QProcessEnvironment env;
    QStringList::ConstIterator it = list.constBegin(),
                              end = list.constEnd();
    for ( ; it != end; ++it) {
        const qsizetype pos = it->indexOf(u'=', 1);
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
    const OrderedNameMapMutexLocker locker(this, &other);
    auto nit = other.nameMap.constBegin();
    const auto nend = other.nameMap.constEnd();
    for ( ; nit != nend; ++nit)
        nameMap.insert(nit.key(), nit.value());
#endif
}

/*!
    \enum QProcessEnvironment::Initialization

    This enum contains a token that is used to disambiguate constructors.

    \value InheritFromParent A QProcessEnvironment will be created that, when
        set on a QProcess, causes it to inherit variables from its parent.

    \since 6.3
*/

/*!
    Creates a new QProcessEnvironment object. This constructor creates an
    empty environment. If set on a QProcess, this will cause the current
    environment variables to be removed (except for PATH and SystemRoot
    on Windows).
*/
QProcessEnvironment::QProcessEnvironment() : d(new QProcessEnvironmentPrivate) { }

/*!
    Creates an object that when set on QProcess will cause it to be executed with
    environment variables inherited from its parent process.

    \note The created object does not store any environment variables by itself,
    it just indicates to QProcess to arrange for inheriting the environment at the
    time when the new process is started. Adding any environment variables to
    the created object will disable inheritance of the environment and result in
    an environment containing only the added environment variables.

    If a modified version of the parent environment is wanted, start with the
    return value of \c systemEnvironment() and modify that (but note that changes to
    the parent process's environment after that is created won't be reflected
    in the modified environment).

    \sa inheritsFromParent(), systemEnvironment()
    \since 6.3
*/
QProcessEnvironment::QProcessEnvironment(QProcessEnvironment::Initialization) noexcept { }

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
    \memberswap{process environment instance}
*/

/*!
    \fn bool QProcessEnvironment::operator!=(const QProcessEnvironment &lhs, const QProcessEnvironment &rhs)

    Returns \c true if the process environment objects \a lhs and \a rhs are different.

    \sa operator==()
*/

/*!
   \fn bool QProcessEnvironment::operator==(const QProcessEnvironment &lhs, const QProcessEnvironment &rhs)

    Returns \c true if the process environment objects \a lhs and \a rhs are equal.

    Two QProcessEnvironment objects are considered equal if they have the same
    set of key=value pairs. The comparison of keys is done case-sensitive on
    platforms where the environment is case-sensitive.

    \sa operator!=(), contains()
*/
bool comparesEqual(const QProcessEnvironment &lhs, const QProcessEnvironment &rhs)
{
    if (lhs.d == rhs.d)
        return true;

    return lhs.d && rhs.d && lhs.d->vars == rhs.d->vars;
}

/*!
    Returns \c true if this QProcessEnvironment object is empty: that is
    there are no key=value pairs set.

    This method also returns \c true for objects that were constructed using
    \c{QProcessEnvironment::InheritFromParent}.

    \sa clear(), systemEnvironment(), insert(), inheritsFromParent()
*/
bool QProcessEnvironment::isEmpty() const
{
    // Needs no locking, as no hash nodes are accessed
    return d ? d->vars.isEmpty() : true;
}

/*!
    Returns \c true if this QProcessEnvironment was constructed using
    \c{QProcessEnvironment::InheritFromParent}.

    \since 6.3
    \sa isEmpty()
*/
bool QProcessEnvironment::inheritsFromParent() const
{
    return !d;
}

/*!
    Removes all key=value pairs from this QProcessEnvironment object, making
    it empty.

    If the environment was constructed using \c{QProcessEnvironment::InheritFromParent}
    it remains unchanged.

    \sa isEmpty(), systemEnvironment()
*/
void QProcessEnvironment::clear()
{
    if (d.constData())
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
    if (d.constData()) {
        QProcessEnvironmentPrivate *p = d.data();
        p->vars.remove(p->prepareName(name));
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
    return d->toList();
}

/*!
    \since 4.8

    Returns a list containing all the variable names in this QProcessEnvironment
    object.

    The returned list is empty for objects constructed using
    \c{QProcessEnvironment::InheritFromParent}.
*/
QStringList QProcessEnvironment::keys() const
{
    if (!d)
        return QStringList();
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
    d->insert(*e.d);
}

#if QT_CONFIG(process)

void QProcessPrivate::Channel::clear()
{
    switch (type) {
    case PipeSource:
        Q_ASSERT(process);
        process->stdinChannel.type = Normal;
        process->stdinChannel.process = nullptr;
        break;
    case PipeSink:
        Q_ASSERT(process);
        process->stdoutChannel.type = Normal;
        process->stdoutChannel.process = nullptr;
        break;
    default:
        break;
    }

    type = Normal;
    file.clear();
    process = nullptr;
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

    \note QProcess is not supported on VxWorks, iOS, tvOS, or watchOS.

    \section1 Finding the Executable

    The program to be run can be set either by calling setProgram() or directly
    in the start() call. The effect of calling start() with the program name
    and arguments is equivalent to calling setProgram() and setArguments()
    before that function and then calling the overload without those
    parameters.

    QProcess interprets the program name in one of three different ways,
    similar to how Unix shells and the Windows command interpreter operate in
    their own command-lines:

    \list
      \li If the program name is an absolute path, then that is the exact
      executable that will be launched and QProcess performs no searching.

      \li If the program name is a relative path with more than one path
      component (that is, it contains at least one slash), the starting
      directory where that relative path is searched is OS-dependent: on
      Windows, it's the parent process' current working dir, while on Unix it's
      the one set with setWorkingDirectory().

      \li If the program name is a plain file name with no slashes, the
      behavior is operating-system dependent. On Unix systems, QProcess will
      search the \c PATH environment variable; on Windows, the search is
      performed by the OS and will first the parent process' current directory
      before the \c PATH environment variable (see the documentation for
      \l{CreateProcess} for the full list).
    \endlist

    To avoid platform-dependent behavior or any issues with how the current
    application was launched, it is advisable to always pass an absolute path
    to the executable to be launched. For auxiliary binaries shipped with the
    application, one can construct such a path starting with
    QCoreApplication::applicationDirPath(). Similarly, to explicitly run an
    executable that is to be found relative to the directory set with
    setWorkingDirectory(), use a program path starting with "./" or "../" as
    the case may be.

    On Windows, the ".exe" suffix is not required for most uses, except those
    outlined in the \l{CreateProcess} documentation. Additionally, QProcess
    will convert the Unix-style forward slashes to Windows path backslashes for
    the program name. This allows code using QProcess to be written in a
    cross-platform manner, as shown in the examples above.

    QProcess does not support directly executing Unix shell or Windows command
    interpreter built-in functions, such as \c{cmd.exe}'s \c dir command or the
    Bourne shell's \c export. On Unix, even though many shell built-ins are
    also provided as separate executables, their behavior may differ from those
    implemented as built-ins. To run those commands, one should explicitly
    execute the interpreter with suitable options. For Unix systems, launch
    "/bin/sh" with two arguments: "-c" and a string with the command-line to be
    run. For Windows, due to the non-standard way \c{cmd.exe} parses its
    command-line, use setNativeArguments() (for example, "/c dir d:").

    \section1 Environment variables

    The QProcess API offers methods to manipulate the environment variables
    that the child process will see. By default, the child process will have a
    copy of the current process environment variables that exist at the time
    the start() function is called. This means that any modifications performed
    using qputenv() prior to that call will be reflected in the child process'
    environment. Note that QProcess makes no attempt to prevent race conditions
    with qputenv() happening in other threads, so it is recommended to avoid
    qputenv() after the application's initial start up.

    The environment for a specific child can be modified using the
    processEnvironment() and setProcessEnvironment() functions, which use the
    \l QProcessEnvironment class. By default, processEnvironment() will return
    an object for which QProcessEnvironment::inheritsFromParent() is true.
    Setting an environment that does not inherit from the parent will cause
    QProcess to use exactly that environment for the child when it is started.

    The normal scenario starts from the current environment by calling
    QProcessEnvironment::systemEnvironment() and then proceeds to adding,
    changing, or removing specific variables. The resulting variable roster can
    then be applied to a QProcess with setProcessEnvironment().

    It is possible to remove all variables from the environment or to start
    from an empty environment, using the QProcessEnvironment() default
    constructor. This is not advisable outside of controlled and
    system-specific conditions, as there may be system variables that are set
    in the current process environment and are required for proper execution
    of the child process.

    On Windows, QProcess will copy the current process' \c "PATH" and \c
    "SystemRoot" environment variables if they were unset. It is not possible
    to unset them completely, but it is possible to set them to empty values.
    Setting \c "PATH" to empty on Windows will likely cause the child process
    to fail to start.

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
    are interleaved. For detached processes, the merged output of the
    running process is forwarded onto the main process.

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
    permissions or resources to invoke the program.

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
    \inmodule QtCore
    \note This struct is only available on the Windows platform.

    This struct is a representation of all parameters of the Windows API
    function \c CreateProcess. It is used as parameter for
    \c CreateProcessArgumentModifier functions.

    \sa QProcess::CreateProcessArgumentModifier
*/

/*!
    \class QProcess::UnixProcessParameters
    \inmodule QtCore
    \note This struct is only available on Unix platforms
    \since 6.6

    This struct can be used to pass extra, Unix-specific configuration for the
    child process using QProcess::setUnixProcessParameters().

    Its members are:
    \list
    \li UnixProcessParameters::flags    Flags, see QProcess::UnixProcessFlags
    \li UnixProcessParameters::lowestFileDescriptorToClose  The lowest file
            descriptor to close.
    \endlist

    When the QProcess::UnixProcessFlags::CloseFileDescriptors flag is set in
    the \c flags field, QProcess closes the application's open file descriptors
    before executing the child process. The descriptors 0, 1, and 2 (that is,
    \c stdin, \c stdout, and \c stderr) are left alone, along with the ones
    numbered lower than the value of the \c lowestFileDescriptorToClose field.

    All of the settings above can also be manually achieved by calling the
    respective POSIX function from a handler set with
    QProcess::setChildProcessModifier(). This structure allows QProcess to deal
    with any platform-specific differences, benefit from certain optimizations,
    and reduces code duplication. Moreover, if any of those functions fail,
    QProcess will enter QProcess::FailedToStart state, while the child process
    modifier callback is not allowed to fail.

    \sa QProcess::setUnixProcessParameters(), QProcess::setChildProcessModifier()
*/

/*!
    \enum QProcess::UnixProcessFlag
    \since 6.6

    These flags can be used in the \c flags field of \l UnixProcessParameters.

    \value CloseFileDescriptors  Close all file descriptors above the threshold
           defined by \c lowestFileDescriptorToClose, preventing any currently
           open descriptor in the parent process from accidentally leaking to the
           child. The \c stdin, \c stdout, and \c stderr file descriptors are
           never closed.

    \value [since 6.7] CreateNewSession  Starts a new process session, by calling
           \c{setsid(2)}. This allows the child process to outlive the session
           the current process is in. This is one of the steps that
           startDetached() takes to allow the process to detach, and is also one
           of the steps to daemonize a process.

    \value [since 6.7] DisconnectControllingTerminal   Requests that the process
           disconnect from its controlling terminal, if it has one. If it has
           none, nothing happens. Processes still connected to a controlling
           terminal may get a Hang Up (\c SIGHUP) signal if the terminal
           closes, or one of the other terminal-control signals (\c SIGTSTP, \c
           SIGTTIN, \c SIGTTOU). Note that on some operating systems, a process
           may only disconnect from the controlling terminal if it is the
           session leader, meaning the \c CreateNewSession flag may be
           required. Like it, this is one of the steps to daemonize a process.

    \value IgnoreSigPipe    Always sets the \c SIGPIPE signal to ignored
           (\c SIG_IGN), even if the \c ResetSignalHandlers flag was set. By
           default, if the child attempts to write to its standard output or
           standard error after the respective channel was closed with
           QProcess::closeReadChannel(), it would get the \c SIGPIPE signal and
           terminate immediately; with this flag, the write operation fails
           without a signal and the child may continue executing.

    \value [since 6.7] ResetIds     Drops any retained, effective user or group
           ID the current process may still have (see \c{setuid(2)} and
           \c{setgid(2)}, plus QCoreApplication::setSetuidAllowed()). This is
           useful if the current process was setuid or setgid and does not wish
           the child process to retain the elevated privileges.

    \value ResetSignalHandlers  Resets all Unix signal handlers back to their
           default state (that is, pass \c SIG_DFL to \c{signal(2)}). This flag
           is useful to ensure any ignored (\c SIG_IGN) signal does not affect
           the child's behavior.

    \value UseVFork  Requests that QProcess use \c{vfork(2)} to start the child
           process. Use this flag to indicate that the callback function set
           with setChildProcessModifier() is safe to execute in the child side of
           a \c{vfork(2)}; that is, the callback does not modify any non-local
           variables (directly or through any function it calls), nor attempts
           to communicate with the parent process. It is implementation-defined
           if QProcess will actually use \c{vfork(2)} and if \c{vfork(2)} is
           different from standard \c{fork(2)}.

    \value [since 6.9] DisableCoreDumps     Requests that QProcess disable core
           dumps in the child process. This is useful if the executable being
           run is likely to crash but users and maintainers are going to be
           uninterested in generating bug reports for those conditions (for
           example, the executable is a test process). This setting does not
           affect the exitStatus() of the crashed process. It is implemented
           by setting the core dump size resource soft limit to zero, meaning
           the application can still reverse this change by raising it to a
           value up to the hard limit.

    \sa setUnixProcessParameters(), unixProcessParameters()
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
#ifndef Q_OS_WIN
    writeBufferChunkSize = QRINGBUFFER_CHUNKSIZE;
#endif
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
    emit q->errorOccurred(QProcess::ProcessError(processError));
}

/*!
    \internal
*/
bool QProcessPrivate::openChannels()
{
    // stdin channel.
    if (inputChannelMode == QProcess::ForwardedInputChannel) {
        if (stdinChannel.type != Channel::Normal)
            qWarning("QProcess::openChannels: Inconsistent stdin channel configuration");
    } else if (!openChannel(stdinChannel)) {
        return false;
    }

    // stdout channel.
    if (processChannelMode == QProcess::ForwardedChannels
            || processChannelMode == QProcess::ForwardedOutputChannel) {
        if (stdoutChannel.type != Channel::Normal)
            qWarning("QProcess::openChannels: Inconsistent stdout channel configuration");
    } else if (!openChannel(stdoutChannel)) {
        return false;
    }

    // stderr channel.
    if (processChannelMode == QProcess::ForwardedChannels
            || processChannelMode == QProcess::ForwardedErrorChannel
            || processChannelMode == QProcess::MergedChannels) {
        if (stderrChannel.type != Channel::Normal)
            qWarning("QProcess::openChannels: Inconsistent stderr channel configuration");
    } else if (!openChannel(stderrChannel)) {
        return false;
    }

    return true;
}

/*!
    \internal
*/
void QProcessPrivate::closeChannels()
{
    closeChannel(&stdoutChannel);
    closeChannel(&stderrChannel);
    closeChannel(&stdinChannel);
}

/*!
    \internal
*/
bool QProcessPrivate::openChannelsForDetached()
{
    // stdin channel.
    bool needToOpen = (stdinChannel.type == Channel::Redirect
                       || stdinChannel.type == Channel::PipeSink);
    if (stdinChannel.type != Channel::Normal
            && (!needToOpen
                || inputChannelMode == QProcess::ForwardedInputChannel)) {
        qWarning("QProcess::openChannelsForDetached: Inconsistent stdin channel configuration");
    }
    if (needToOpen && !openChannel(stdinChannel))
        return false;

    // stdout channel.
    needToOpen = (stdoutChannel.type == Channel::Redirect
                  || stdoutChannel.type == Channel::PipeSource);
    if (stdoutChannel.type != Channel::Normal
            && (!needToOpen
                || processChannelMode == QProcess::ForwardedChannels
                || processChannelMode == QProcess::ForwardedOutputChannel)) {
        qWarning("QProcess::openChannelsForDetached: Inconsistent stdout channel configuration");
    }
    if (needToOpen && !openChannel(stdoutChannel))
        return false;

    // stderr channel.
    needToOpen = (stderrChannel.type == Channel::Redirect);
    if (stderrChannel.type != Channel::Normal
            && (!needToOpen
                || processChannelMode == QProcess::ForwardedChannels
                || processChannelMode == QProcess::ForwardedErrorChannel
                || processChannelMode == QProcess::MergedChannels)) {
        qWarning("QProcess::openChannelsForDetached: Inconsistent stderr channel configuration");
    }
    if (needToOpen && !openChannel(stderrChannel))
        return false;

    return true;
}

/*!
    \internal
    Returns \c true if we emitted readyRead().
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
        closeChannel(channel);
#if defined QPROCESS_DEBUG
        qDebug("QProcessPrivate::tryReadFromChannel(%d), 0 bytes available",
               int(channel - &stdinChannel));
#endif
        return false;
    }
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::tryReadFromChannel(%d), read %lld bytes from the process' output",
           int(channel - &stdinChannel), readBytes);
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
            QScopedValueRollback<bool> guard(emittedReadyRead, true);
            emit q->readyRead();
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
void QProcessPrivate::_q_processDied()
{
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::_q_processDied()");
#endif

    // in case there is data in the pipeline and this slot by chance
    // got called before the read notifications, call these functions
    // so the data is made available before we announce death.
#ifdef Q_OS_WIN
    drainOutputPipes();
#else
    _q_canReadStandardOutput();
    _q_canReadStandardError();
#endif

    // Slots connected to signals emitted by the functions called above
    // might call waitFor*(), which would synchronously reap the process.
    // So check the state to avoid trying to reap a second time.
    if (processState != QProcess::NotRunning)
        processFinished();
}

/*!
    \internal
*/
void QProcessPrivate::processFinished()
{
    Q_Q(QProcess);
#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::processFinished()");
#endif

#ifdef Q_OS_UNIX
    waitForDeadChild();
#else
    findExitCode();
#endif

    cleanup();

    if (exitStatus == QProcess::CrashExit)
        setErrorAndEmit(QProcess::Crashed);

    // we received EOF now:
    emit q->readChannelFinished();
    // in the future:
    //emit q->standardOutputClosed();
    //emit q->standardErrorClosed();

    emit q->finished(exitCode, QProcess::ExitStatus(exitStatus));

#if defined QPROCESS_DEBUG
    qDebug("QProcessPrivate::processFinished(): process is dead");
#endif
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

    QString errorMessage;
    if (processStarted(&errorMessage)) {
        q->setProcessState(QProcess::Running);
        emit q->started(QProcess::QPrivateSignal());
        return true;
    }

    q->setProcessState(QProcess::NotRunning);
    setErrorAndEmit(QProcess::FailedToStart, errorMessage);
#ifdef Q_OS_UNIX
    waitForDeadChild();
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
    d->cleanup();
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
    return ProcessChannelMode(d->processChannelMode);
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
    return InputChannelMode(d->inputChannelMode);
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
    if (bytesToWrite() == 0)
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
    read channel is closed: reading from it using \l read() will always
    fail, as will \l readAllStandardOutput().

    To discard all standard output from the process, pass \l nullDevice()
    here. This is more efficient than simply never reading the standard
    output, as no QProcess buffers are filled.

    If the file \a fileName doesn't exist at the moment \l start() is
    called, it will be created. If it cannot be created, the starting
    will fail.

    If the file exists and \a mode is \ QIODeviceBase::Truncate, the file
    will be truncated. Otherwise (if \a mode is \l QIODeviceBase::Append),
    the file will be appended to.

    Calling \l setStandardOutputFile() after the process has started has
    no effect.

    If \a fileName is an empty string, it stops redirecting the standard
    output. This is useful for restoring the standard output after redirection.

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

#if defined(Q_OS_WIN) || defined(Q_QDOC)

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

    \sa QProcess::CreateProcessArgumentModifier, setChildProcessModifier()
*/
void QProcess::setCreateProcessArgumentsModifier(CreateProcessArgumentModifier modifier)
{
    Q_D(QProcess);
    d->modifyCreateProcessArgs = modifier;
}

#endif

#if defined(Q_OS_UNIX) || defined(Q_QDOC)
/*!
    \since 6.0

    Returns the modifier function previously set by calling
    setChildProcessModifier().

    \note This function is only available on Unix platforms.

    \sa setChildProcessModifier(), unixProcessParameters()
*/
std::function<void(void)> QProcess::childProcessModifier() const
{
    Q_D(const QProcess);
    return d->unixExtras ? d->unixExtras->childProcessModifier : std::function<void(void)>();
}

/*!
    \since 6.0

    Sets the \a modifier function for the child process, for Unix systems
    (including \macos; for Windows, see setCreateProcessArgumentsModifier()).
    The function contained by the \a modifier argument will be invoked in the
    child process after \c{fork()} or \c{vfork()} is completed and QProcess has
    set up the standard file descriptors for the child process, but before
    \c{execve()}, inside start().

    The following shows an example of setting up a child process to run without
    privileges:

    \snippet code/src_corelib_io_qprocess.cpp 4

    If the modifier function experiences a failure condition, it can use
    failChildProcessModifier() to report the situation to the QProcess caller.
    Alternatively, it may use other methods of stopping the process, like
    \c{_exit()}, or \c{abort()}.

    Certain properties of the child process, such as closing all extraneous
    file descriptors or disconnecting from the controlling TTY, can be more
    readily achieved by using setUnixProcessParameters(), which can detect
    failure and report a \l{QProcess::}{FailedToStart} condition. The modifier
    is useful to change certain uncommon properties of the child process, such
    as setting up additional file descriptors. If both a child process modifier
    and Unix process parameters are set, the modifier is run before these
    parameters are applied.

    \note In multithreaded applications, this function must be careful not to
    call any functions that may lock mutexes that may have been in use in
    other threads (in general, using only functions defined by POSIX as
    "async-signal-safe" is advised). Most of the Qt API is unsafe inside this
    callback, including qDebug(), and may lead to deadlocks.

    \note If the UnixProcessParameters::UseVFork flag is set via
    setUnixProcessParameters(), QProcess may use \c{vfork()} semantics to
    start the child process, so this function must obey even stricter
    constraints. First, because it is still sharing memory with the parent
    process, it must not write to any non-local variable and must obey proper
    ordering semantics when reading from them, to avoid data races. Second,
    even more library functions may misbehave; therefore, this function should
    only make use of low-level system calls, such as \c{read()},
    \c{write()}, \c{setsid()}, \c{nice()}, and similar.

    \sa childProcessModifier(), failChildProcessModifier(), setUnixProcessParameters()
*/
void QProcess::setChildProcessModifier(const std::function<void(void)> &modifier)
{
    Q_D(QProcess);
    if (!d->unixExtras)
        d->unixExtras.reset(new QProcessPrivate::UnixExtras);
    d->unixExtras->childProcessModifier = modifier;
}

/*!
    \fn void QProcess::failChildProcessModifier(const char *description, int error) noexcept
    \since 6.7

    This functions can be used inside the modifier set with
    setChildProcessModifier() to indicate an error condition was encountered.
    When the modifier calls these functions, QProcess will emit errorOccurred()
    with code QProcess::FailedToStart in the parent process. The \a description
    can be used to include some information in errorString() to help diagnose
    the problem, usually the name of the call that failed, similar to the C
    Library function \c{perror()}. Additionally, the \a error parameter can be
    an \c{<errno.h>} error code whose text form will also be included.

    For example, a child modifier could prepare an extra file descriptor for
    the child process this way:

    \code
        process.setChildProcessModifier([fd, &process]() {
            if (dup2(fd, TargetFileDescriptor) < 0)
                process.failChildProcessModifier(errno, "aux comm channel");
        });
        process.start();
    \endcode

    Where \c{fd} is a file descriptor currently open in the parent process. If
    the \c{dup2()} system call resulted in an \c EBADF condition, the process
    errorString() could be "Child process modifier reported error: aux comm
    channel: Bad file descriptor".

    This function does not return to the caller. Using it anywhere except in
    the child modifier and with the correct QProcess object is undefined
    behavior.

    \note The implementation imposes a length limit to the \a description
    parameter to about 500 characters. This does not include the text from the
    \a error code.

    \sa setChildProcessModifier(), setUnixProcessParameters()
*/

/*!
    \since 6.6
    Returns the \l UnixProcessParameters object describing extra flags and
    settings that will be applied to the child process on Unix systems. The
    default settings correspond to a default-constructed UnixProcessParameters.

    \note This function is only available on Unix platforms.

    \sa childProcessModifier()
*/
auto QProcess::unixProcessParameters() const noexcept -> UnixProcessParameters
{
    Q_D(const QProcess);
    return d->unixExtras ? d->unixExtras->processParameters : UnixProcessParameters{};
}

/*!
    \since 6.6
    Sets the extra settings and parameters for the child process on Unix
    systems to be \a params. This function can be used to ask QProcess to
    modify the child process before launching the target executable.

    This function can be used to change certain properties of the child
    process, such as closing all extraneous file descriptors, changing the nice
    level of the child, or disconnecting from the controlling TTY. For more
    fine-grained control of the child process or to modify it in other ways,
    use the setChildProcessModifier() function. If both a child process
    modifier and Unix process parameters are set, the modifier is run before
    these parameters are applied.

    \note This function is only available on Unix platforms.

    \sa unixProcessParameters(), setChildProcessModifier()
*/
void QProcess::setUnixProcessParameters(const UnixProcessParameters &params)
{
    Q_D(QProcess);
    if (!d->unixExtras)
        d->unixExtras.reset(new QProcessPrivate::UnixExtras);
    d->unixExtras->processParameters = params;
}

/*!
    \since 6.6
    \overload

    Sets the extra settings for the child process on Unix systems to \a
    flagsOnly. This is the same as the overload with just the \c flags field
    set.
    \note This function is only available on Unix platforms.

    \sa unixProcessParameters(), setChildProcessModifier()
*/
void QProcess::setUnixProcessParameters(UnixProcessFlags flagsOnly)
{
    Q_D(QProcess);
    if (!d->unixExtras)
        d->unixExtras.reset(new QProcessPrivate::UnixExtras);
    d->unixExtras->processParameters = { flagsOnly };
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

    \sa workingDirectory(), start()
*/
void QProcess::setWorkingDirectory(const QString &dir)
{
    Q_D(QProcess);
    d->workingDirectory = dir;
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
*/
bool QProcess::isSequential() const
{
    return true;
}

/*! \reimp
*/
qint64 QProcess::bytesToWrite() const
{
#ifdef Q_OS_WIN
    return d_func()->pipeWriterBytesToWrite();
#else
    return QIODevice::bytesToWrite();
#endif
}

/*!
    Returns the type of error that occurred last.

    \sa state()
*/
QProcess::ProcessError QProcess::error() const
{
    Q_D(const QProcess);
    return ProcessError(d->processError);
}

/*!
    Returns the current state of the process.

    \sa stateChanged(), error()
*/
QProcess::ProcessState QProcess::state() const
{
    Q_D(const QProcess);
    return ProcessState(d->processState);
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

    \sa processEnvironment(), QProcessEnvironment::systemEnvironment(),
        {Environment variables}
*/
void QProcess::setProcessEnvironment(const QProcessEnvironment &environment)
{
    Q_D(QProcess);
    d->environment = environment;
}

/*!
    \since 4.6
    Returns the environment that QProcess will pass to its child process. If no
    environment has been set using setProcessEnvironment(), this method returns
    an object indicating the environment will be inherited from the parent.

    \sa setProcessEnvironment(), QProcessEnvironment::inheritsFromParent(),
        {Environment variables}
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
    occurred). If the process had already started successfully before this
    function, it returns immediately.

    This function can operate without an event loop. It is
    useful when writing non-GUI applications and when performing
    I/O operations in a non-GUI thread.

    \warning Calling this function from the main (GUI) thread
    might cause your user interface to freeze.

    If msecs is -1, this function will not time out.

    \sa started(), waitForReadyRead(), waitForBytesWritten(), waitForFinished()
*/
bool QProcess::waitForStarted(int msecs)
{
    Q_D(QProcess);
    if (d->processState == QProcess::Starting)
        return d->waitForStarted(QDeadlineTimer(msecs));

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

    QDeadlineTimer deadline(msecs);
    if (d->processState == QProcess::Starting) {
        bool started = d->waitForStarted(deadline);
        if (!started)
            return false;
    }

    return d->waitForReadyRead(deadline);
}

/*! \reimp
*/
bool QProcess::waitForBytesWritten(int msecs)
{
    Q_D(QProcess);
    if (d->processState == QProcess::NotRunning)
        return false;

    QDeadlineTimer deadline(msecs);
    if (d->processState == QProcess::Starting) {
        bool started = d->waitForStarted(deadline);
        if (!started)
            return false;
    }

    return d->waitForBytesWritten(deadline);
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

    QDeadlineTimer deadline(msecs);
    if (d->processState == QProcess::Starting) {
        bool started = d->waitForStarted(deadline);
        if (!started)
            return false;
    }

    return d->waitForFinished(deadline);
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

#if QT_VERSION < QT_VERSION_CHECK(7,0,0)
/*!
    \internal
*/
auto QProcess::setupChildProcess() -> Use_setChildProcessModifier_Instead
{
    Q_UNREACHABLE_RETURN({});
}
#endif

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
    Q_D(QProcess);
    QByteArray data;
    if (d->processChannelMode == MergedChannels) {
        qWarning("QProcess::readAllStandardError: Called with MergedChannels");
    } else {
        ProcessChannel tmp = readChannel();
        setReadChannel(StandardError);
        data = readAll();
        setReadChannel(tmp);
    }
    return data;
}

/*!
    Starts the given \a program in a new process, passing the command line
    arguments in \a arguments. See setProgram() for information about how
    QProcess searches for the executable to be run. The OpenMode is set to \a
    mode. No further splitting of the arguments is performed.

    The QProcess object will immediately enter the Starting state. If the
    process starts successfully, QProcess will emit started(); otherwise,
    errorOccurred() will be emitted. Do note that on platforms that are able to
    start child processes synchronously (notably Windows), those signals will
    be emitted before this function returns and this QProcess object will
    transition to either QProcess::Running or QProcess::NotRunning state,
    respectively. On others paltforms, the started() and errorOccurred()
    signals will be delayed.

    Call waitForStarted() to make sure the process has started (or has failed
    to start) and those signals have been emitted. It is safe to call that
    function even if the process starting state is already known, though the
    signal will not be emitted again.

    \b{Windows:} The arguments are quoted and joined into a command line
    that is compatible with the \c CommandLineToArgvW() Windows function.
    For programs that have different command line quoting requirements,
    you need to use setNativeArguments(). One notable program that does
    not follow the \c CommandLineToArgvW() rules is cmd.exe and, by
    consequence, all batch scripts.

    If the QProcess object is already running a process, a warning may be
    printed at the console, and the existing process will continue running
    unaffected.

    \note Success at starting the child process only implies the operating
    system has successfully created the process and assigned the resources
    every process has, such as its process ID. The child process may crash or
    otherwise fail very early and thus not produce its expected output. On most
    operating systems, this may include dynamic linking errors.

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
    \since 6.0

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
    behaves like start().

    On operating systems where the system API for passing command line
    arguments to a subprocess natively uses a single string (Windows), one can
    conceive command lines which cannot be passed via QProcess's portable
    list-based API. In these rare cases you need to use setProgram() and
    setNativeArguments() instead of this function.

    \sa splitCommand()
    \sa start()
 */
void QProcess::startCommand(const QString &command, OpenMode mode)
{
    QStringList args = splitCommand(command);
    if (args.isEmpty()) {
        qWarning("QProcess::startCommand: empty or whitespace-only command was provided");
        return;
    }
    const QString program = args.takeFirst();
    start(program, args, mode);
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

    If the function is successful then *\a pid is set to the process identifier
    of the started process; otherwise, it's set to -1. Note that the child
    process may exit and the PID may become invalid without notice.
    Furthermore, after the child process exits, the same PID may be recycled
    and used by a completely different process. User code should be careful
    when using this variable, especially if one intends to forcibly terminate
    the process by operating system means.

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
    \li setProcessChannelMode(QProcess::MergedChannels)
    \li setStandardOutputProcess()
    \li setWorkingDirectory()
    \endlist
    All other properties of the QProcess object are ignored.

    \note The called process inherits the console window of the calling
    process. To suppress console output, redirect standard/error output to
    QProcess::nullDevice().

    \sa start()
    \sa startDetached(const QString &program, const QStringList &arguments,
                      const QString &workingDirectory, qint64 *pid)
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

    Returns \c true if the program has been started.

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
#endif // QT_CONFIG(process)

/*!
    \since 5.15

    Splits the string \a command into a list of tokens, and returns
    the list.

    Tokens with spaces can be surrounded by double quotes; three
    consecutive double quotes represent the quote character itself.
*/
QStringList QProcess::splitCommand(QStringView command)
{
    QStringList args;
    QString tmp;
    int quoteCount = 0;
    bool inQuote = false;

    // handle quoting. tokens can be surrounded by double quotes
    // "hello world". three consecutive double quotes represent
    // the quote character itself.
    for (int i = 0; i < command.size(); ++i) {
        if (command.at(i) == u'"') {
            ++quoteCount;
            if (quoteCount == 3) {
                // third consecutive quote
                quoteCount = 0;
                tmp += command.at(i);
            }
            continue;
        }
        if (quoteCount) {
            if (quoteCount == 1)
                inQuote = !inQuote;
            quoteCount = 0;
        }
        if (!inQuote && command.at(i).isSpace()) {
            if (!tmp.isEmpty()) {
                args += tmp;
                tmp.clear();
            }
        } else {
            tmp += command.at(i);
        }
    }
    if (!tmp.isEmpty())
        args += tmp;

    return args;
}

#if QT_CONFIG(process)
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

    If \a program is an absolute path, it specifies the exact executable that
    will be launched. Relative paths will be resolved in a platform-specific
    manner, which includes searching the \c PATH environment variable (see
    \l{Finding the Executable} for details).

    \sa start(), setArguments(), program(), QStandardPaths::findExecutable()
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
    return ExitStatus(d->exitStatus);
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
    process.setProcessChannelMode(ForwardedChannels);
    process.start(program, arguments);
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
    return QProcessEnvironment::systemEnvironment().toStringList();
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

#endif // QT_CONFIG(process)

QT_END_NAMESPACE

#if QT_CONFIG(process)
#include "moc_qprocess.cpp"
#endif
