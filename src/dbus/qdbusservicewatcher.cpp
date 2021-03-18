/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtDBus module of the Qt Toolkit.
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

#include "qdbusservicewatcher.h"
#include "qdbusconnection.h"
#include "qdbusutil_p.h"

#include <QStringList>

#include <private/qobject_p.h>
#include <private/qdbusconnection_p.h>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

class QDBusServiceWatcherPrivate: public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QDBusServiceWatcher)
public:
    QDBusServiceWatcherPrivate(const QDBusConnection &c, QDBusServiceWatcher::WatchMode wm)
        : connection(c), watchMode(wm)
    {
    }

    QStringList servicesWatched;
    QDBusConnection connection;
    QDBusServiceWatcher::WatchMode watchMode;

    void _q_serviceOwnerChanged(const QString &, const QString &, const QString &);
    void setConnection(const QStringList &services, const QDBusConnection &c, QDBusServiceWatcher::WatchMode watchMode);

    void addService(const QString &service);
    void removeService(const QString &service);
};

void QDBusServiceWatcherPrivate::_q_serviceOwnerChanged(const QString &service, const QString &oldOwner, const QString &newOwner)
{
    Q_Q(QDBusServiceWatcher);
    emit q->serviceOwnerChanged(service, oldOwner, newOwner);
    if (oldOwner.isEmpty())
        emit q->serviceRegistered(service);
    else if (newOwner.isEmpty())
        emit q->serviceUnregistered(service);
}

void QDBusServiceWatcherPrivate::setConnection(const QStringList &s, const QDBusConnection &c, QDBusServiceWatcher::WatchMode wm)
{
    if (connection.isConnected()) {
        // remove older rules
        for (const QString &s : qAsConst(servicesWatched))
            removeService(s);
    }

    connection = c;
    watchMode = wm;
    servicesWatched = s;

    if (connection.isConnected()) {
        // add new rules
        for (const QString &s : qAsConst(servicesWatched))
            addService(s);
    }
}

void QDBusServiceWatcherPrivate::addService(const QString &service)
{
    QDBusConnectionPrivate *d = QDBusConnectionPrivate::d(connection);
    if (d && d->shouldWatchService(service))
        d->watchService(service, watchMode, q_func(), SLOT(_q_serviceOwnerChanged(QString,QString,QString)));
}

void QDBusServiceWatcherPrivate::removeService(const QString &service)
{
    QDBusConnectionPrivate *d = QDBusConnectionPrivate::d(connection);
    if (d && d->shouldWatchService(service))
        d->unwatchService(service, watchMode, q_func(), SLOT(_q_serviceOwnerChanged(QString,QString,QString)));
}

/*!
    \class QDBusServiceWatcher
    \since 4.6
    \inmodule QtDBus

    \brief The QDBusServiceWatcher class allows the user to watch for a bus service change.

    A QDBusServiceWatcher object can be used to notify the application about
    an ownership change of a service name on the bus. It has three watch
    modes:

    \list
      \li Watching for service registration only.
      \li Watching for service unregistration only.
      \li Watching for any kind of service ownership change (the default mode).
    \endlist

    Besides being created or deleted, services may change owners without a
    unregister/register operation happening. So the serviceRegistered()
    and serviceUnregistered() signals may not be emitted if that
    happens.

    This class is more efficient than using the
    QDBusConnectionInterface::serviceOwnerChanged() signal because it allows
    one to receive only the signals for which the class is interested in.

    Ending a service name with the character '*' will match all service names
    within the specified namespace.

    For example "com.example.backend1*" will match
    \list
        \li com.example.backend1
        \li com.example.backend1.foo
        \li com.example.backend1.foo.bar
    \endlist
    Substrings in the same domain will not be matched, i.e "com.example.backend12".

    \sa QDBusConnection
*/

/*!
    \enum QDBusServiceWatcher::WatchModeFlag

    QDBusServiceWatcher supports three different watch modes, which are configured by this flag:

    \value WatchForRegistration watch for service registration only, ignoring
    any signals related to other service ownership change.

    \value WatchForUnregistration watch for service unregistration only,
    ignoring any signals related to other service ownership change.

    \value WatchForOwnerChange watch for any kind of service ownership
    change.
*/

/*!
    \property QDBusServiceWatcher::watchMode

    The \c watchMode property holds the current watch mode for this
    QDBusServiceWatcher object. The default value for this property is
    QDBusServiceWatcher::WatchForOwnershipChange.
*/

/*!
    \property QDBusServiceWatcher::watchedServices

    The \c servicesWatched property holds the list of services watched.

    Note that modifying this list with setServicesWatched() is an expensive
    operation. If you can, prefer to change it by way of addWatchedService()
    and removeWatchedService().
*/

/*!
    \fn void QDBusServiceWatcher::serviceRegistered(const QString &serviceName)

    This signal is emitted whenever this object detects that the service \a
    serviceName became available on the bus.

    \sa serviceUnregistered(), serviceOwnerChanged()
*/

/*!
    \fn void QDBusServiceWatcher::serviceUnregistered(const QString &serviceName)

    This signal is emitted whenever this object detects that the service \a
    serviceName was unregistered from the bus and is no longer available.

    \sa serviceRegistered(), serviceOwnerChanged()
*/

/*!
    \fn void QDBusServiceWatcher::serviceOwnerChanged(const QString &serviceName, const QString &oldOwner, const QString &newOwner)

    This signal is emitted whenever this object detects that there was a
    service ownership change relating to the \a serviceName service. The \a
    oldOwner parameter contains the old owner name and \a newOwner is the new
    owner. Both \a oldOwner and \a newOwner are unique connection names.

    Note that this signal is also emitted whenever the \a serviceName service
    was registered or unregistered. If it was registered, \a oldOwner will
    contain an empty string, whereas if it was unregistered, \a newOwner will
    contain an empty string.

    If you need only to find out if the service is registered or unregistered
    only, without being notified that the ownership changed, consider using
    the specific modes for those operations. This class is more efficient if
    you use the more specific modes.

    \sa serviceRegistered(), serviceUnregistered()
*/

/*!
    Creates a QDBusServiceWatcher object. Note that until you set a
    connection with setConnection(), this object will not emit any signals.

    The \a parent parameter is passed to QObject to set the parent of this
    object.
*/
QDBusServiceWatcher::QDBusServiceWatcher(QObject *parent)
    : QObject(*new QDBusServiceWatcherPrivate(QDBusConnection(QString()), WatchForOwnerChange), parent)
{
}

/*!
    Creates a QDBusServiceWatcher object and attaches it to the \a connection
    connection. Also, this function immediately starts watching for \a
    watchMode changes to service \a service.

    The \a parent parameter is passed to QObject to set the parent of this
    object.
*/
QDBusServiceWatcher::QDBusServiceWatcher(const QString &service, const QDBusConnection &connection, WatchMode watchMode, QObject *parent)
    : QObject(*new QDBusServiceWatcherPrivate(connection, watchMode), parent)
{
    d_func()->setConnection(QStringList() << service, connection, watchMode);
}

/*!
    Destroys the QDBusServiceWatcher object and releases any resources
    associated with it.
*/
QDBusServiceWatcher::~QDBusServiceWatcher()
{
}

/*!
    Returns the list of D-Bus services that are being watched.

    \sa setWatchedServices()
*/
QStringList QDBusServiceWatcher::watchedServices() const
{
    return d_func()->servicesWatched;
}

/*!
    Sets the list of D-Bus services being watched to be \a services.

    Note that setting the entire list means removing all previous rules for
    watching services and adding new ones. This is an expensive operation and
    should be avoided, if possible. Instead, use addWatchedService() and
    removeWatchedService() if you can to manipulate entries in the list.
*/
void QDBusServiceWatcher::setWatchedServices(const QStringList &services)
{
    Q_D(QDBusServiceWatcher);
    if (services == d->servicesWatched)
        return;
    d->setConnection(services, d->connection, d->watchMode);
}

/*!
    Adds \a newService to the list of services to be watched by this object.
    This function is more efficient than setWatchedServices() and should be
    used whenever possible to add services.
*/
void QDBusServiceWatcher::addWatchedService(const QString &newService)
{
    Q_D(QDBusServiceWatcher);
    if (d->servicesWatched.contains(newService))
        return;
    d->addService(newService);
    d->servicesWatched << newService;
}

/*!
    Removes the \a service from the list of services being watched by this
    object. Note that D-Bus notifications are asynchronous, so there may
    still be signals pending delivery about \a service. Those signals will
    still be emitted whenever the D-Bus messages are processed.

    This function returns \c true if any services were removed.
*/
bool QDBusServiceWatcher::removeWatchedService(const QString &service)
{
    Q_D(QDBusServiceWatcher);
    d->removeService(service);
    return d->servicesWatched.removeOne(service);
}

QDBusServiceWatcher::WatchMode QDBusServiceWatcher::watchMode() const
{
    return d_func()->watchMode;
}

void QDBusServiceWatcher::setWatchMode(WatchMode mode)
{
    Q_D(QDBusServiceWatcher);
    if (mode == d->watchMode)
        return;
    d->setConnection(d->servicesWatched, d->connection, mode);
}

/*!
    Returns the QDBusConnection that this object is attached to.

    \sa setConnection()
*/
QDBusConnection QDBusServiceWatcher::connection() const
{
    return d_func()->connection;
}

/*!
    Sets the D-Bus connection that this object is attached to be \a
    connection. All services watched will be transferred to this connection.

    Note that QDBusConnection objects are reference counted:
    QDBusServiceWatcher will keep a reference for this connection while it
    exists. The connection is not closed until the reference count drops to
    zero, so this will ensure that any notifications are received while this
    QDBusServiceWatcher object exists.

    \sa connection()
*/
void QDBusServiceWatcher::setConnection(const QDBusConnection &connection)
{
    Q_D(QDBusServiceWatcher);
    if (connection.name() == d->connection.name())
        return;
    d->setConnection(d->servicesWatched, connection, d->watchMode);
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS

#include "moc_qdbusservicewatcher.cpp"
