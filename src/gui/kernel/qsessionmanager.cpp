/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtGui module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#include <qsessionmanager.h>
#include <qguiapplication.h>

#include <private/qobject_p.h>

#ifndef QT_NO_SESSIONMANAGER

QT_BEGIN_NAMESPACE

/*!
    \class QSessionManager
    \brief The QSessionManager class provides access to the session manager.

    \inmodule QtWidgets

    A session manager in a desktop environment (in which Qt GUI applications
    live) keeps track of a session, which is a group of running applications,
    each of which has a particular state. The state of an application contains
    (most notably) the documents the application has open and the position and
    size of its windows.

    The session manager is used to save the session, e.g., when the machine is
    shut down, and to restore a session, e.g., when the machine is started up.
    We recommend that you use QSettings to save an application's settings,
    for example, window positions, recently used files, etc. When the
    application is restarted by the session manager, you can restore the
    settings.

    QSessionManager provides an interface between the application and the
    session manager so that the program can work well with the session manager.
    In Qt, session management requests for action are handled by the two
    virtual functions QApplication::commitData() and QApplication::saveState().
    Both provide a reference to a session manager object as argument, to allow
    the application to communicate with the session manager. The session
    manager can only be accessed through these functions.

    No user interaction is possible \e unless the application gets explicit
    permission from the session manager. You ask for permission by calling
    allowsInteraction() or, if it is really urgent, allowsErrorInteraction().
    Qt does not enforce this, but the session manager may.

    You can try to abort the shutdown process by calling cancel(). The default
    commitData() function does this if some top-level window rejected its
    closeEvent().

    For sophisticated session managers provided on Unix/X11, QSessionManager
    offers further possibilities to fine-tune an application's session
    management behavior: setRestartCommand(), setDiscardCommand(),
    setRestartHint(), setProperty(), requestPhase2(). See the respective
    function descriptions for further details.

    \sa QApplication, {Session Management}
*/


/*! \enum QSessionManager::RestartHint

    This enum type defines the circumstances under which this application wants
    to be restarted by the session manager. The current values are:

    \value  RestartIfRunning    If the application is still running when the
                                session is shut down, it wants to be restarted
                                at the start of the next session.

    \value  RestartAnyway       The application wants to be started at the
                                start of the next session, no matter what.
                                (This is useful for utilities that run just
                                after startup and then quit.)

    \value  RestartImmediately  The application wants to be started immediately
                                whenever it is not running.

    \value  RestartNever        The application does not want to be restarted
                                automatically.

    The default hint is \c RestartIfRunning.
*/


class QSessionManagerPrivate : public QObjectPrivate
{
public:
    QSessionManagerPrivate(QSessionManager *m, const QString &id,
                           const QString &key);

    QStringList restartCommand;
    QStringList discardCommand;
    const QString sessionId;
    const QString sessionKey;
    QSessionManager::RestartHint restartHint;
};

QSessionManagerPrivate::QSessionManagerPrivate(QSessionManager*,
                                               const QString &id,
                                               const QString &key)
    : QObjectPrivate(), sessionId(id), sessionKey(key)
{
}

QSessionManager::QSessionManager(QGuiApplication *app, QString &id, QString &key)
    : QObject(*(new QSessionManagerPrivate(this, id, key)), app)
{
    Q_D(QSessionManager);
    d->restartHint = RestartIfRunning;
}

QSessionManager::~QSessionManager()
{
}

/*!
    Returns the identifier of the current session.

    If the application has been restored from an earlier session, this
    identifier is the same as it was in the earlier session.

    \sa sessionKey(), QApplication::sessionId()
*/
QString QSessionManager::sessionId() const
{
    Q_D(const QSessionManager);
    return d->sessionId;
}

/*!
    \fn QString QSessionManager::sessionKey() const

    Returns the session key in the current session.

    If the application has been restored from an earlier session, this key is
    the same as it was when the previous session ended.

    The session key changes with every call of commitData() or saveState().

    \sa sessionId(), QApplication::sessionKey()
*/
QString QSessionManager::sessionKey() const
{
    Q_D(const QSessionManager);
    return d->sessionKey;
}


/*!
    Asks the session manager for permission to interact with the user. Returns
    true if interaction is permitted; otherwise returns false.

    The rationale behind this mechanism is to make it possible to synchronize
    user interaction during a shutdown. Advanced session managers may ask all
    applications simultaneously to commit their data, resulting in a much
    faster shutdown.

    When the interaction is completed we strongly recommend releasing the user
    interaction semaphore with a call to release(). This way, other
    applications may get the chance to interact with the user while your
    application is still busy saving data. (The semaphore is implicitly
    released when the application exits.)

    If the user decides to cancel the shutdown process during the interaction
    phase, you must tell the session manager that this has happened by calling
    cancel().

    Here's an example of how an application's QApplication::commitData() might
    be implemented:

    \snippet code/src_gui_kernel_qapplication.cpp 8

    If an error occurred within the application while saving its data, you may
    want to try allowsErrorInteraction() instead.

    \sa QApplication::commitData(), release(), cancel()
*/
bool QSessionManager::allowsInteraction()
{
    return false;
}

/*!
    Returns true if error interaction is permitted; otherwise returns false.

    This is similar to allowsInteraction(), but also enables the application to
    tell the user about any errors that occur. Session managers may give error
    interaction requests higher priority, which means that it is more likely
    that an error interaction is permitted. However, you are still not
    guaranteed that the session manager will allow interaction.

    \sa allowsInteraction(), release(), cancel()
*/
bool QSessionManager::allowsErrorInteraction()
{
    return false;
}

/*!
    Releases the session manager's interaction semaphore after an interaction
    phase.

    \sa allowsInteraction(), allowsErrorInteraction()
*/
void QSessionManager::release()
{
}

/*!
    Tells the session manager to cancel the shutdown process.  Applications
    should not call this function without asking the user first.

    \sa allowsInteraction(), allowsErrorInteraction()
*/
void QSessionManager::cancel()
{
}

/*!
    Sets the application's restart hint to \a hint. On application startup, the
    hint is set to \c RestartIfRunning.

    \note These flags are only hints, a session manager may or may not respect
    them.

    We recommend setting the restart hint in QApplication::saveState() because
    most session managers perform a checkpoint shortly after an application's
    startup.

    \sa restartHint()
*/
void QSessionManager::setRestartHint(QSessionManager::RestartHint hint)
{
    Q_D(QSessionManager);
    d->restartHint = hint;
}

/*!
    \fn QSessionManager::RestartHint QSessionManager::restartHint() const

    Returns the application's current restart hint. The default is
    \c RestartIfRunning.

    \sa setRestartHint()
*/
QSessionManager::RestartHint QSessionManager::restartHint() const
{
    Q_D(const QSessionManager);
    return d->restartHint;
}

/*!
    If the session manager is capable of restoring sessions it will execute
    \a command in order to restore the application. The command defaults to

    \snippet code/src_gui_kernel_qapplication.cpp 9

    The \c -session option is mandatory; otherwise QApplication cannot tell
    whether it has been restored or what the current session identifier is.
    See QApplication::isSessionRestored() and QApplication::sessionId() for
    details.

    If your application is very simple, it may be possible to store the entire
    application state in additional command line options. This is usually a
    very bad idea because command lines are often limited to a few hundred
    bytes. Instead, use QSettings, temporary files, or a database for this
    purpose. By marking the data with the unique sessionId(), you will be able
    to restore the application in a future  session.

    \sa restartCommand(), setDiscardCommand(), setRestartHint()
*/
void QSessionManager::setRestartCommand(const QStringList &command)
{
    Q_D(QSessionManager);
    d->restartCommand = command;
}

/*!
    Returns the currently set restart command.

    To iterate over the list, you can use the \l foreach pseudo-keyword:

    \snippet code/src_gui_kernel_qapplication.cpp 10

    \sa setRestartCommand(), restartHint()
*/
QStringList QSessionManager::restartCommand() const
{
    Q_D(const QSessionManager);
    return d->restartCommand;
}

/*!
    Sets the discard command to the given \a command.

    \sa discardCommand(), setRestartCommand()
*/
void QSessionManager::setDiscardCommand(const QStringList &command)
{
    Q_D(QSessionManager);
    d->discardCommand = command;
}

/*!
    Returns the currently set discard command.

    To iterate over the list, you can use the \l foreach pseudo-keyword:

    \snippet code/src_gui_kernel_qapplication.cpp 11

    \sa setDiscardCommand(), restartCommand(), setRestartCommand()
*/
QStringList QSessionManager::discardCommand() const
{
    Q_D(const QSessionManager);
    return d->discardCommand;
}

/*!
    \overload

    Low-level write access to the application's identification and state
    records are kept in the session manager.

    The property called \a name has its value set to the string \a value.
*/
void QSessionManager::setManagerProperty(const QString &name,
                                         const QString &value)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
}

/*!
    Low-level write access to the application's identification and state record
    are kept in the session manager.

    The property called \a name has its value set to the string list \a value.
*/
void QSessionManager::setManagerProperty(const QString &name,
                                         const QStringList &value)
{
    Q_UNUSED(name);
    Q_UNUSED(value);
}

/*!
    Returns true if the session manager is currently performing a second
    session management phase; otherwise returns false.

    \sa requestPhase2()
*/
bool QSessionManager::isPhase2() const
{
    return false;
}

/*!
    Requests a second session management phase for the application. The
    application may then return immediately from the QApplication::commitData()
    or QApplication::saveState() function, and they will be called again once
    most or all other applications have finished their session management.

    The two phases are useful for applications such as the X11 window manager
    that need to store information about another application's windows and
    therefore have to wait until these applications have completed their
    respective session management tasks.

    \note If another application has requested a second phase it may get called
    before, simultaneously with, or after your application's second phase.

    \sa isPhase2()
*/
void QSessionManager::requestPhase2()
{
}

QT_END_NAMESPACE

#endif // QT_NO_SESSIONMANAGER
