/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Copyright (C) 2016 Intel Corporation.
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

#include "qdbusintegrator_p.h"

#include <qcoreapplication.h>
#include <qelapsedtimer.h>
#include <qdebug.h>
#include <qmetaobject.h>
#include <qobject.h>
#include <qsocketnotifier.h>
#include <qstringlist.h>
#include <qtimer.h>
#include <qthread.h>
#include <private/qlocking_p.h>

#include "qdbusargument.h"
#include "qdbusconnection_p.h"
#include "qdbusconnectionmanager_p.h"
#include "qdbusinterface_p.h"
#include "qdbusmessage.h"
#include "qdbusmetatype.h"
#include "qdbusmetatype_p.h"
#include "qdbusabstractadaptor.h"
#include "qdbusabstractadaptor_p.h"
#include "qdbusserver.h"
#include "qdbusutil_p.h"
#include "qdbusvirtualobject.h"
#include "qdbusmessage_p.h"
#include "qdbuscontext_p.h"
#include "qdbuspendingcall_p.h"

#include "qdbusthreaddebug_p.h"

#include <algorithm>
#ifdef interface
#undef interface
#endif

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

// used with dbus_server_allocate_data_slot
static dbus_int32_t server_slot = -1;

static QBasicAtomicInt isDebugging = Q_BASIC_ATOMIC_INITIALIZER(-1);
#define qDBusDebug              if (::isDebugging == 0); else qDebug

static inline QDebug operator<<(QDebug dbg, const QThread *th)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QThread(ptr=" << (const void*)th;
    if (th && !th->objectName().isEmpty())
        dbg.nospace() << ", name=" << th->objectName();
    else if (th)
        dbg.nospace() << ", name=" << th->metaObject()->className();
    dbg.nospace() << ')';
    return dbg;
}

#if QDBUS_THREAD_DEBUG
static inline QDebug operator<<(QDebug dbg, const QDBusConnectionPrivate *conn)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "QDBusConnection("
                  << "ptr=" << (const void*)conn
                  << ", name=" << conn->name
                  << ", baseService=" << conn->baseService
                  << ')';
    return dbg;
}

void qdbusDefaultThreadDebug(int action, int condition, QDBusConnectionPrivate *conn)
{
    qDBusDebug() << QThread::currentThread()
                 << "Qt D-Bus threading action" << action
                 << (condition == QDBusLockerBase::BeforeLock ? "before lock" :
                     condition == QDBusLockerBase::AfterLock ? "after lock" :
                     condition == QDBusLockerBase::BeforeUnlock ? "before unlock" :
                     condition == QDBusLockerBase::AfterUnlock ? "after unlock" :
                     condition == QDBusLockerBase::BeforePost ? "before event posting" :
                     condition == QDBusLockerBase::AfterPost ? "after event posting" :
                     condition == QDBusLockerBase::BeforeDeliver ? "before event delivery" :
                     condition == QDBusLockerBase::AfterDeliver ? "after event delivery" :
                     condition == QDBusLockerBase::BeforeAcquire ? "before acquire" :
                     condition == QDBusLockerBase::AfterAcquire ? "after acquire" :
                     condition == QDBusLockerBase::BeforeRelease ? "before release" :
                     condition == QDBusLockerBase::AfterRelease ? "after release" :
                     "condition unknown")
                 << "in connection" << conn;
}
qdbusThreadDebugFunc qdbusThreadDebug = nullptr;
#endif

typedef QVarLengthArray<QDBusSpyCallEvent::Hook, 4> QDBusSpyHookList;
Q_GLOBAL_STATIC(QDBusSpyHookList, qDBusSpyHookList)

extern "C" {

    // libdbus-1 callbacks

static dbus_bool_t qDBusAddTimeout(DBusTimeout *timeout, void *data)
{
    Q_ASSERT(timeout);
    Q_ASSERT(data);

  //  qDebug("addTimeout %d", q_dbus_timeout_get_interval(timeout));

    QDBusConnectionPrivate *d = static_cast<QDBusConnectionPrivate *>(data);
    Q_ASSERT(QThread::currentThread() == d->thread());

    // we may get called from qDBusToggleTimeout
    if (Q_UNLIKELY(!q_dbus_timeout_get_enabled(timeout)))
        return false;

    Q_ASSERT(d->timeouts.key(timeout, 0) == 0);

    int timerId = d->startTimer(q_dbus_timeout_get_interval(timeout));
    Q_ASSERT_X(timerId, "QDBusConnection", "Failed to start a timer");
    if (!timerId)
        return false;

    d->timeouts[timerId] = timeout;
    return true;
}

static void qDBusRemoveTimeout(DBusTimeout *timeout, void *data)
{
    Q_ASSERT(timeout);
    Q_ASSERT(data);

  //  qDebug("removeTimeout");

    QDBusConnectionPrivate *d = static_cast<QDBusConnectionPrivate *>(data);
    Q_ASSERT(QThread::currentThread() == d->thread());

    QDBusConnectionPrivate::TimeoutHash::iterator it = d->timeouts.begin();
    while (it != d->timeouts.end()) {
        if (it.value() == timeout) {
            d->killTimer(it.key());
            it = d->timeouts.erase(it);
            break;
        } else {
            ++it;
        }
    }
}

static void qDBusToggleTimeout(DBusTimeout *timeout, void *data)
{
    Q_ASSERT(timeout);
    Q_ASSERT(data);

    //qDebug("ToggleTimeout");

    qDBusRemoveTimeout(timeout, data);
    qDBusAddTimeout(timeout, data);
}

static dbus_bool_t qDBusAddWatch(DBusWatch *watch, void *data)
{
    Q_ASSERT(watch);
    Q_ASSERT(data);

    QDBusConnectionPrivate *d = static_cast<QDBusConnectionPrivate *>(data);
    Q_ASSERT(QThread::currentThread() == d->thread());

    int flags = q_dbus_watch_get_flags(watch);
    int fd = q_dbus_watch_get_unix_fd(watch);

    QDBusConnectionPrivate::Watcher watcher;

    if (flags & DBUS_WATCH_READABLE) {
        //qDebug("addReadWatch %d", fd);
        watcher.watch = watch;
        watcher.read = new QSocketNotifier(fd, QSocketNotifier::Read, d);
        watcher.read->setEnabled(q_dbus_watch_get_enabled(watch));
        d->connect(watcher.read, &QSocketNotifier::activated, d, &QDBusConnectionPrivate::socketRead);
    }
    if (flags & DBUS_WATCH_WRITABLE) {
        //qDebug("addWriteWatch %d", fd);
        watcher.watch = watch;
        watcher.write = new QSocketNotifier(fd, QSocketNotifier::Write, d);
        watcher.write->setEnabled(q_dbus_watch_get_enabled(watch));
        d->connect(watcher.write, &QSocketNotifier::activated, d, &QDBusConnectionPrivate::socketWrite);
    }
    d->watchers.insert(fd, watcher);

    return true;
}

static void qDBusRemoveWatch(DBusWatch *watch, void *data)
{
    Q_ASSERT(watch);
    Q_ASSERT(data);

    //qDebug("remove watch");

    QDBusConnectionPrivate *d = static_cast<QDBusConnectionPrivate *>(data);
    Q_ASSERT(QThread::currentThread() == d->thread());
    int fd = q_dbus_watch_get_unix_fd(watch);

    QDBusConnectionPrivate::WatcherHash::iterator i = d->watchers.find(fd);
    while (i != d->watchers.end() && i.key() == fd) {
        if (i.value().watch == watch) {
            delete i.value().read;
            delete i.value().write;
            i = d->watchers.erase(i);
        } else {
            ++i;
        }
    }
}

static void qDBusToggleWatch(DBusWatch *watch, void *data)
{
    Q_ASSERT(watch);
    Q_ASSERT(data);

    QDBusConnectionPrivate *d = static_cast<QDBusConnectionPrivate *>(data);
    Q_ASSERT(QThread::currentThread() == d->thread());
    int fd = q_dbus_watch_get_unix_fd(watch);

    QDBusConnectionPrivate::WatcherHash::iterator i = d->watchers.find(fd);
    while (i != d->watchers.end() && i.key() == fd) {
        if (i.value().watch == watch) {
            bool enabled = q_dbus_watch_get_enabled(watch);
            int flags = q_dbus_watch_get_flags(watch);

            //qDebug("toggle watch %d to %d (write: %d, read: %d)", q_dbus_watch_get_fd(watch), enabled, flags & DBUS_WATCH_WRITABLE, flags & DBUS_WATCH_READABLE);

            if (flags & DBUS_WATCH_READABLE && i.value().read)
                i.value().read->setEnabled(enabled);
            if (flags & DBUS_WATCH_WRITABLE && i.value().write)
                i.value().write->setEnabled(enabled);
            return;
        }
        ++i;
    }
}

static void qDBusUpdateDispatchStatus(DBusConnection *connection, DBusDispatchStatus new_status, void *data)
{
    Q_ASSERT(connection);
    Q_UNUSED(connection);
    QDBusConnectionPrivate *d = static_cast<QDBusConnectionPrivate *>(data);
    if (new_status == DBUS_DISPATCH_DATA_REMAINS)
        emit d->dispatchStatusChanged();
}

static void qDBusNewConnection(DBusServer *server, DBusConnection *connection, void *data)
{
    // ### We may want to separate the server from the QDBusConnectionPrivate
    Q_ASSERT(server); Q_UNUSED(server);
    Q_ASSERT(connection);
    Q_ASSERT(data);

    if (!QDBusConnectionManager::instance())
        return;

    // keep the connection alive
    q_dbus_connection_ref(connection);
    QDBusConnectionPrivate *serverConnection = static_cast<QDBusConnectionPrivate *>(data);

    // allow anonymous authentication
    if (serverConnection->anonymousAuthenticationAllowed)
        q_dbus_connection_set_allow_anonymous(connection, true);

    QDBusConnectionPrivate *newConnection = new QDBusConnectionPrivate(serverConnection->parent());
    const auto locker = qt_scoped_lock(QDBusConnectionManager::instance()->mutex);
    QDBusConnectionManager::instance()->setConnection(QLatin1String("QDBusServer-") + QString::number(reinterpret_cast<qulonglong>(newConnection), 16), newConnection);
    serverConnection->serverConnectionNames << newConnection->name;

    // setPeer does the error handling for us
    QDBusErrorInternal error;
    newConnection->setPeer(connection, error);
    newConnection->setDispatchEnabled(false);

    // this is a queued connection and will resume in the QDBusServer's thread
    emit serverConnection->newServerConnection(newConnection);

    // we've disabled dispatching of events, so now we post an event to the
    // QDBusServer's thread in order to enable it after the
    // QDBusServer::newConnection() signal has been received by the
    // application's code
    newConnection->ref.ref();
    QReadLocker serverLock(&serverConnection->lock);
    QDBusConnectionDispatchEnabler *o = new QDBusConnectionDispatchEnabler(newConnection);
    QTimer::singleShot(0, o, SLOT(execute()));
    if (serverConnection->serverObject)
        o->moveToThread(serverConnection->serverObject->thread());
}

void QDBusConnectionPrivate::_q_newConnection(QDBusConnectionPrivate *newConnection)
{
    Q_ASSERT(mode == ServerMode);
    emit serverObject->newConnection(QDBusConnectionPrivate::q(newConnection));
}

} // extern "C"

static QByteArray buildMatchRule(const QString &service,
                                 const QString &objectPath, const QString &interface,
                                 const QString &member, const QDBusConnectionPrivate::ArgMatchRules &argMatch, const QString & /*signature*/)
{
    QString result;
    result += QLatin1String("type='signal',");
    const auto keyValue = QLatin1String("%1='%2',");

    if (!service.isEmpty())
        result += keyValue.arg(QLatin1String("sender"), service);
    if (!objectPath.isEmpty())
        result += keyValue.arg(QLatin1String("path"), objectPath);
    if (!interface.isEmpty())
        result += keyValue.arg(QLatin1String("interface"), interface);
    if (!member.isEmpty())
        result += keyValue.arg(QLatin1String("member"), member);

    // add the argument string-matching now
    if (!argMatch.args.isEmpty()) {
        const QString keyValue = QLatin1String("arg%1='%2',");
        for (int i = 0; i < argMatch.args.count(); ++i)
            if (!argMatch.args.at(i).isNull())
                result += keyValue.arg(i).arg(argMatch.args.at(i));
    }
    if (!argMatch.arg0namespace.isEmpty()) {
        result += QLatin1String("arg0namespace='%1',").arg(argMatch.arg0namespace);
    }

    result.chop(1);             // remove ending comma
    return result.toLatin1();
}

static bool findObject(const QDBusConnectionPrivate::ObjectTreeNode *root,
                       const QString &fullpath, int &usedLength,
                       QDBusConnectionPrivate::ObjectTreeNode &result)
{
    if (!fullpath.compare(QLatin1String("/")) && root->obj) {
        usedLength = 1;
        result = *root;
        return root;
    }
    int start = 0;
    int length = fullpath.length();
    if (fullpath.at(0) == QLatin1Char('/'))
        start = 1;

    // walk the object tree
    QDBusConnectionPrivate::ObjectTreeNode::DataList::ConstIterator node = root;
    while (start < length && node) {
        if (node->flags & QDBusConnection::ExportChildObjects)
            break;
        if ((node->flags & QDBusConnectionPrivate::VirtualObject) && (node->flags & QDBusConnection::SubPath))
            break;
        int end = fullpath.indexOf(QLatin1Char('/'), start);
        end = (end == -1 ? length : end);
        QStringRef pathComponent(&fullpath, start, end - start);

        QDBusConnectionPrivate::ObjectTreeNode::DataList::ConstIterator it =
            std::lower_bound(node->children.constBegin(), node->children.constEnd(), pathComponent);
        if (it != node->children.constEnd() && it->name == pathComponent)
            // match
            node = it;
        else
            node = nullptr;

        start = end + 1;
    }

    // found our object
    usedLength = (start > length ? length : start);
    if (node) {
        if (node->obj || !node->children.isEmpty())
            result = *node;
        else
            // there really is no object here
            // we're just looking at an unused space in the QVector
            node = nullptr;
    }
    return node;
}

static QObject *findChildObject(const QDBusConnectionPrivate::ObjectTreeNode *root,
                                const QString &fullpath, int start)
{
    int length = fullpath.length();

    // any object in the tree can tell us to switch to its own object tree:
    const QDBusConnectionPrivate::ObjectTreeNode *node = root;
    if (node && node->flags & QDBusConnection::ExportChildObjects) {
        QObject *obj = node->obj;

        while (obj) {
            if (start >= length)
                // we're at the correct level
                return obj;

            int pos = fullpath.indexOf(QLatin1Char('/'), start);
            pos = (pos == -1 ? length : pos);
            QStringRef pathComponent(&fullpath, start, pos - start);

            const QObjectList children = obj->children();

            // find a child with the proper name
            QObject *next = nullptr;
            QObjectList::ConstIterator it = children.constBegin();
            QObjectList::ConstIterator end = children.constEnd();
            for ( ; it != end; ++it)
                if ((*it)->objectName() == pathComponent) {
                    next = *it;
                    break;
                }

            if (!next)
                break;

            obj = next;
            start = pos + 1;
        }
    }

    // object not found
    return nullptr;
}

static QDBusConnectionPrivate::ArgMatchRules matchArgsForService(const QString &service, QDBusServiceWatcher::WatchMode mode)
{
    QDBusConnectionPrivate::ArgMatchRules matchArgs;
    if (service.endsWith(QLatin1Char('*'))) {
        matchArgs.arg0namespace = service.chopped(1);
        matchArgs.args << QString();
    }
    else
        matchArgs.args << service;

    switch (mode) {
    case QDBusServiceWatcher::WatchForOwnerChange:
        break;

    case QDBusServiceWatcher::WatchForRegistration:
        matchArgs.args << QString::fromLatin1("", 0);
        break;

    case QDBusServiceWatcher::WatchForUnregistration:
        matchArgs.args << QString() << QString::fromLatin1("", 0);
        break;
    }
    return matchArgs;
}


extern Q_DBUS_EXPORT void qDBusAddSpyHook(QDBusSpyCallEvent::Hook);
void qDBusAddSpyHook(QDBusSpyCallEvent::Hook hook)
{
    qDBusSpyHookList()->append(hook);
}

QDBusSpyCallEvent::~QDBusSpyCallEvent()
{
    // Reinsert the message into the processing queue for the connection.
    // This is done in the destructor so the message is reinserted even if
    // QCoreApplication is destroyed.
    QDBusConnectionPrivate *d = static_cast<QDBusConnectionPrivate *>(const_cast<QObject *>(sender()));
    qDBusDebug() << d << "message spies done for" << msg;
    emit d->spyHooksFinished(msg);
}

void QDBusSpyCallEvent::placeMetaCall(QObject *)
{
    invokeSpyHooks(msg, hooks, hookCount);
}

inline void QDBusSpyCallEvent::invokeSpyHooks(const QDBusMessage &msg, const Hook *hooks, int hookCount)
{
    // call the spy hook list
    for (int i = 0; i < hookCount; ++i)
        hooks[i](msg);
}

extern "C" {
static DBusHandlerResult
qDBusSignalFilter(DBusConnection *connection, DBusMessage *message, void *data)
{
    Q_ASSERT(data);
    Q_UNUSED(connection);
    QDBusConnectionPrivate *d = static_cast<QDBusConnectionPrivate *>(data);
    if (d->mode == QDBusConnectionPrivate::InvalidMode)
        return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

    QDBusMessage amsg = QDBusMessagePrivate::fromDBusMessage(message, d->connectionCapabilities());
    qDBusDebug() << d << "got message (signal):" << amsg;

    return d->handleMessage(amsg) ?
        DBUS_HANDLER_RESULT_HANDLED :
        DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}
}

bool QDBusConnectionPrivate::handleMessage(const QDBusMessage &amsg)
{
    if (!ref.loadRelaxed())
        return false;

    // local message are always delivered, regardless of filtering
    // or whether the dispatcher is enabled
    bool isLocal = QDBusMessagePrivate::isLocal(amsg);

    if (!dispatchEnabled && !isLocal) {
        // queue messages only, we'll handle them later
        qDBusDebug() << this << "delivery is suspended";
        pendingMessages << amsg;
        return amsg.type() == QDBusMessage::MethodCallMessage;
    }

    switch (amsg.type()) {
    case QDBusMessage::SignalMessage:
        handleSignal(amsg);
        // if there are any other filters in this DBusConnection,
        // let them see the signal too
        return false;
    case QDBusMessage::MethodCallMessage:
        // run it through the spy filters (if any) before the regular processing:
        // a) if it's a local message, we're in the caller's thread, so invoke the filter directly
        // b) if it's an external message, post to the main thread
        if (Q_UNLIKELY(qDBusSpyHookList.exists()) && qApp) {
            const QDBusSpyHookList &list = *qDBusSpyHookList;
            if (isLocal) {
                Q_ASSERT(QThread::currentThread() != thread());
                qDBusDebug() << this << "invoking message spies directly";
                QDBusSpyCallEvent::invokeSpyHooks(amsg, list.constData(), list.size());
            } else {
                qDBusDebug() << this << "invoking message spies via event";
                QCoreApplication::postEvent(qApp, new QDBusSpyCallEvent(this, QDBusConnection(this),
                                                                        amsg, list.constData(), list.size()));

                // we'll be called back, so return
                return true;
            }
        }

        handleObjectCall(amsg);
        return true;
    case QDBusMessage::ReplyMessage:
    case QDBusMessage::ErrorMessage:
    case QDBusMessage::InvalidMessage:
        return false;           // we don't handle those here
    }

    return false;
}

static void huntAndDestroy(QObject *needle, QDBusConnectionPrivate::ObjectTreeNode &haystack)
{
    for (auto &node : haystack.children)
        huntAndDestroy(needle, node);

    auto isInactive = [](QDBusConnectionPrivate::ObjectTreeNode &node) { return !node.isActive(); };

    haystack.children.erase(std::remove_if(haystack.children.begin(), haystack.children.end(),
                                           isInactive),
                            haystack.children.end());

    if (needle == haystack.obj) {
        haystack.obj = nullptr;
        haystack.flags = 0;
    }
}

static void huntAndUnregister(const QVector<QStringRef> &pathComponents, int i, QDBusConnection::UnregisterMode mode,
                              QDBusConnectionPrivate::ObjectTreeNode *node)
{
    if (pathComponents.count() == i) {
        // found it
        node->obj = nullptr;
        node->flags = 0;

        if (mode == QDBusConnection::UnregisterTree) {
            // clear the sub-tree as well
            node->children.clear();  // can't disconnect the objects because we really don't know if they can
                            // be found somewhere else in the path too
        }
    } else {
        // keep going
        QDBusConnectionPrivate::ObjectTreeNode::DataList::Iterator end = node->children.end();
        QDBusConnectionPrivate::ObjectTreeNode::DataList::Iterator it =
            std::lower_bound(node->children.begin(), end, pathComponents.at(i));
        if (it == end || it->name != pathComponents.at(i))
            return;              // node not found

        huntAndUnregister(pathComponents, i + 1, mode, it);
        if (!it->isActive())
            node->children.erase(it);
    }
}

static void huntAndEmit(DBusConnection *connection, DBusMessage *msg,
                        QObject *needle, const QDBusConnectionPrivate::ObjectTreeNode &haystack,
                        bool isScriptable, bool isAdaptor, const QString &path = QString())
{
    QDBusConnectionPrivate::ObjectTreeNode::DataList::ConstIterator it = haystack.children.constBegin();
    QDBusConnectionPrivate::ObjectTreeNode::DataList::ConstIterator end = haystack.children.constEnd();
    for ( ; it != end; ++it) {
        if (it->isActive())
            huntAndEmit(connection, msg, needle, *it, isScriptable, isAdaptor, path + QLatin1Char('/') + it->name);
    }

    if (needle == haystack.obj) {
        // is this a signal we should relay?
        if (isAdaptor && (haystack.flags & QDBusConnection::ExportAdaptors) == 0)
            return;             // no: it comes from an adaptor and we're not exporting adaptors
        else if (!isAdaptor) {
            int mask = isScriptable
                       ? QDBusConnection::ExportScriptableSignals
                       : QDBusConnection::ExportNonScriptableSignals;
            if ((haystack.flags & mask) == 0)
                return;         // signal was not exported
        }

        QByteArray p = path.toLatin1();
        if (p.isEmpty())
            p = "/";
        qDBusDebug() << QThread::currentThread() << "emitting signal at" << p;
        DBusMessage *msg2 = q_dbus_message_copy(msg);
        q_dbus_message_set_path(msg2, p);
        q_dbus_connection_send(connection, msg2, nullptr);
        q_dbus_message_unref(msg2);
    }
}

static int findSlot(const QMetaObject *mo, const QByteArray &name, int flags,
                    const QString &signature_, QVector<int> &metaTypes)
{
    QByteArray msgSignature = signature_.toLatin1();

    for (int idx = mo->methodCount() - 1 ; idx >= QObject::staticMetaObject.methodCount(); --idx) {
        QMetaMethod mm = mo->method(idx);

        // check access:
        if (mm.access() != QMetaMethod::Public)
            continue;

        // check type:
        if (mm.methodType() != QMetaMethod::Slot && mm.methodType() != QMetaMethod::Method)
            continue;

        // check name:
        if (mm.name() != name)
            continue;

        int returnType = mm.returnType();
        bool isAsync = qDBusCheckAsyncTag(mm.tag());
        bool isScriptable = mm.attributes() & QMetaMethod::Scriptable;

        // consistency check:
        if (isAsync && returnType != QMetaType::Void)
            continue;

        QString errorMsg;
        int inputCount = qDBusParametersForMethod(mm, metaTypes, errorMsg);
        if (inputCount == -1)
            continue;           // problem parsing

        metaTypes[0] = returnType;
        bool hasMessage = false;
        if (inputCount > 0 &&
            metaTypes.at(inputCount) == QDBusMetaTypeId::message()) {
            // "no input parameters" is allowed as long as the message meta type is there
            hasMessage = true;
            --inputCount;
        }

        // try to match the parameters
        int i;
        QByteArray reconstructedSignature;
        for (i = 1; i <= inputCount; ++i) {
            const char *typeSignature = QDBusMetaType::typeToSignature( metaTypes.at(i) );
            if (!typeSignature)
                break;          // invalid

            reconstructedSignature += typeSignature;
            if (!msgSignature.startsWith(reconstructedSignature))
                break;
        }

        if (reconstructedSignature != msgSignature)
            continue;           // we didn't match them all

        if (hasMessage)
            ++i;

        // make sure that the output parameters have signatures too
        if (returnType != QMetaType::UnknownType && returnType != QMetaType::Void && QDBusMetaType::typeToSignature(returnType) == nullptr)
            continue;

        bool ok = true;
        for (int j = i; ok && j < metaTypes.count(); ++j)
            if (QDBusMetaType::typeToSignature(metaTypes.at(i)) == nullptr)
                ok = false;
        if (!ok)
            continue;

        // consistency check:
        if (isAsync && metaTypes.count() > i + 1)
            continue;

        if (mm.methodType() == QMetaMethod::Slot) {
            if (isScriptable && (flags & QDBusConnection::ExportScriptableSlots) == 0)
                continue;           // scriptable slots not exported
            if (!isScriptable && (flags & QDBusConnection::ExportNonScriptableSlots) == 0)
                continue;           // non-scriptable slots not exported
        } else {
            if (isScriptable && (flags & QDBusConnection::ExportScriptableInvokables) == 0)
                continue;           // scriptable invokables not exported
            if (!isScriptable && (flags & QDBusConnection::ExportNonScriptableInvokables) == 0)
                continue;           // non-scriptable invokables not exported
        }

        // if we got here, this slot matched
        return idx;
    }

    // no slot matched
    return -1;
}

/*!
    \internal
    Enables or disables the delivery of incoming method calls and signals. If
    \a enable is true, this will also cause any queued, pending messages to be
    delivered.
 */
void QDBusConnectionPrivate::setDispatchEnabled(bool enable)
{
    checkThread();
    dispatchEnabled = enable;
    if (enable)
        emit dispatchStatusChanged();
}

static QDBusCallDeliveryEvent * const DIRECT_DELIVERY = (QDBusCallDeliveryEvent *)1;

QDBusCallDeliveryEvent* QDBusConnectionPrivate::prepareReply(QDBusConnectionPrivate *target,
                                                             QObject *object, int idx,
                                                             const QVector<int> &metaTypes,
                                                             const QDBusMessage &msg)
{
    Q_ASSERT(object);
    Q_UNUSED(object);

    int n = metaTypes.count() - 1;
    if (metaTypes[n] == QDBusMetaTypeId::message())
        --n;

    if (msg.arguments().count() < n)
        return nullptr;               // too few arguments

    // check that types match
    for (int i = 0; i < n; ++i)
        if (metaTypes.at(i + 1) != msg.arguments().at(i).userType() &&
            msg.arguments().at(i).userType() != qMetaTypeId<QDBusArgument>())
            return nullptr;           // no match

    // we can deliver
    // prepare for the call
    if (target == object)
        return DIRECT_DELIVERY;
    return new QDBusCallDeliveryEvent(QDBusConnection(target), idx, target, msg, metaTypes);
}

void QDBusConnectionPrivate::activateSignal(const QDBusConnectionPrivate::SignalHook& hook,
                                            const QDBusMessage &msg)
{
    // This is called by QDBusConnectionPrivate::handleSignal to deliver a signal
    // that was received from D-Bus
    //
    // Signals are delivered to slots if the parameters match
    // Slots can have less parameters than there are on the message
    // Slots can optionally have one final parameter that is a QDBusMessage
    // Slots receive read-only copies of the message (i.e., pass by value or by const-ref)
    QDBusCallDeliveryEvent *call = prepareReply(this, hook.obj, hook.midx, hook.params, msg);
    if (call == DIRECT_DELIVERY) {
        // short-circuit delivery
        Q_ASSERT(this == hook.obj);
        deliverCall(this, 0, msg, hook.params, hook.midx);
        return;
    }
    if (call)
        postEventToThread(ActivateSignalAction, hook.obj, call);
}

bool QDBusConnectionPrivate::activateCall(QObject* object, int flags, const QDBusMessage &msg)
{
    // This is called by QDBusConnectionPrivate::handleObjectCall to place a call
    // to a slot on the object.
    //
    // The call is delivered to the first slot that matches the following conditions:
    //  - has the same name as the message's target member
    //  - ALL of the message's types are found in slot's parameter list
    //  - optionally has one more parameter of type QDBusMessage
    // If none match, then the slot of the same name as the message target and with
    // the first type of QDBusMessage is delivered.
    //
    // The D-Bus specification requires that all MethodCall messages be replied to, unless the
    // caller specifically waived this requirement. This means that we inspect if the user slot
    // generated a reply and, if it didn't, we will. Obviously, if the user slot doesn't take a
    // QDBusMessage parameter, it cannot generate a reply.
    //
    // When a return message is generated, the slot's return type, if any, will be placed
    // in the message's first position. If there are non-const reference parameters to the
    // slot, they must appear at the end and will be placed in the subsequent message
    // positions.

    static const char cachePropertyName[] = "_qdbus_slotCache";

    if (!object)
        return false;

    Q_ASSERT_X(QThread::currentThread() == object->thread(),
               "QDBusConnection: internal threading error",
               "function called for an object that is in another thread!!");

    QDBusSlotCache slotCache =
        qvariant_cast<QDBusSlotCache>(object->property(cachePropertyName));
    QString cacheKey = msg.member(), signature = msg.signature();
    if (!signature.isEmpty()) {
        cacheKey.reserve(cacheKey.length() + 1 + signature.length());
        cacheKey += QLatin1Char('.');
        cacheKey += signature;
    }

    QDBusSlotCache::Hash::ConstIterator cacheIt = slotCache.hash.constFind(cacheKey);
    while (cacheIt != slotCache.hash.constEnd() && cacheIt->flags != flags &&
           cacheIt.key() == cacheKey)
        ++cacheIt;
    if (cacheIt == slotCache.hash.constEnd() || cacheIt.key() != cacheKey)
    {
        // not cached, analyze the meta object
        const QMetaObject *mo = object->metaObject();
        QByteArray memberName = msg.member().toUtf8();

        // find a slot that matches according to the rules above
        QDBusSlotCache::Data slotData;
        slotData.flags = flags;
        slotData.slotIdx = ::findSlot(mo, memberName, flags, msg.signature(), slotData.metaTypes);
        if (slotData.slotIdx == -1) {
            // ### this is where we want to add the connection as an arg too
            // try with no parameters, but with a QDBusMessage
            slotData.slotIdx = ::findSlot(mo, memberName, flags, QString(), slotData.metaTypes);
            if (slotData.metaTypes.count() != 2 ||
                slotData.metaTypes.at(1) != QDBusMetaTypeId::message()) {
                // not found
                // save the negative lookup
                slotData.slotIdx = -1;
                slotData.metaTypes.clear();
                slotCache.hash.insert(cacheKey, slotData);
                object->setProperty(cachePropertyName, QVariant::fromValue(slotCache));
                return false;
            }
        }

        // save to the cache
        slotCache.hash.insert(cacheKey, slotData);
        object->setProperty(cachePropertyName, QVariant::fromValue(slotCache));

        // found the slot to be called
        deliverCall(object, flags, msg, slotData.metaTypes, slotData.slotIdx);
        return true;
    } else if (cacheIt->slotIdx == -1) {
        // negative cache
        return false;
    } else {
        // use the cache
        deliverCall(object, flags, msg, cacheIt->metaTypes, cacheIt->slotIdx);
        return true;
    }
    return false;
}

void QDBusConnectionPrivate::deliverCall(QObject *object, int /*flags*/, const QDBusMessage &msg,
                                         const QVector<int> &metaTypes, int slotIdx)
{
    Q_ASSERT_X(!object || QThread::currentThread() == object->thread(),
               "QDBusConnection: internal threading error",
               "function called for an object that is in another thread!!");

    QVarLengthArray<void *, 10> params;
    params.reserve(metaTypes.count());

    QVariantList auxParameters;
    // let's create the parameter list

    // first one is the return type -- add it below
    params.append(0);

    // add the input parameters
    int i;
    int pCount = qMin(msg.arguments().count(), metaTypes.count() - 1);
    for (i = 1; i <= pCount; ++i) {
        int id = metaTypes[i];
        if (id == QDBusMetaTypeId::message())
            break;

        const QVariant &arg = msg.arguments().at(i - 1);
        if (arg.userType() == id)
            // no conversion needed
            params.append(const_cast<void *>(arg.constData()));
        else if (arg.userType() == qMetaTypeId<QDBusArgument>()) {
            // convert to what the function expects
            void *null = nullptr;
            auxParameters.append(QVariant(id, null));

            const QDBusArgument &in =
                *reinterpret_cast<const QDBusArgument *>(arg.constData());
            QVariant &out = auxParameters[auxParameters.count() - 1];

            if (Q_UNLIKELY(!QDBusMetaType::demarshall(in, out.userType(), out.data())))
                qFatal("Internal error: demarshalling function for type '%s' (%d) failed!",
                       out.typeName(), out.userType());

            params.append(const_cast<void *>(out.constData()));
        } else {
            qFatal("Internal error: got invalid meta type %d (%s) "
                   "when trying to convert to meta type %d (%s)",
                   arg.userType(), QMetaType::typeName(arg.userType()),
                   id, QMetaType::typeName(id));
        }
    }

    if (metaTypes.count() > i && metaTypes[i] == QDBusMetaTypeId::message()) {
        params.append(const_cast<void*>(static_cast<const void*>(&msg)));
        ++i;
    }

    // output arguments
    const int numMetaTypes = metaTypes.count();
    QVariantList outputArgs;
    void *null = nullptr;
    if (metaTypes[0] != QMetaType::Void && metaTypes[0] != QMetaType::UnknownType) {
        outputArgs.reserve(numMetaTypes - i + 1);
        QVariant arg(metaTypes[0], null);
        outputArgs.append( arg );
        params[0] = const_cast<void*>(outputArgs.at( outputArgs.count() - 1 ).constData());
    } else {
        outputArgs.reserve(numMetaTypes - i);
    }

    for ( ; i < numMetaTypes; ++i) {
        QVariant arg(metaTypes[i], null);
        outputArgs.append( arg );
        params.append(const_cast<void*>(outputArgs.at( outputArgs.count() - 1 ).constData()));
    }

    // make call:
    bool fail;
    if (!object) {
        fail = true;
    } else {
        // FIXME: save the old sender!
        QDBusContextPrivate context(QDBusConnection(this), msg);
        QDBusContextPrivate *old = QDBusContextPrivate::set(object, &context);

        QPointer<QObject> ptr = object;
        fail = object->qt_metacall(QMetaObject::InvokeMetaMethod,
                                   slotIdx, params.data()) >= 0;
        // the object might be deleted in the slot
        if (!ptr.isNull())
            QDBusContextPrivate::set(object, old);
    }

    // do we create a reply? Only if the caller is waiting for a reply and one hasn't been sent
    // yet.
    if (msg.isReplyRequired() && !msg.isDelayedReply()) {
        if (!fail) {
            // normal reply
            qDBusDebug() << this << "Automatically sending reply:" << outputArgs;
            send(msg.createReply(outputArgs));
        } else {
            // generate internal error
            qWarning("Internal error: Failed to deliver message");
            send(msg.createErrorReply(QDBusError::InternalError,
                                      QLatin1String("Failed to deliver message")));
        }
    }

    return;
}

extern bool qDBusInitThreads();

QDBusConnectionPrivate::QDBusConnectionPrivate(QObject *p)
    : QObject(p),
      ref(1),
      mode(InvalidMode),
      busService(nullptr),
      connection(nullptr),
      rootNode(QString(QLatin1Char('/'))),
      anonymousAuthenticationAllowed(false),
      dispatchEnabled(true),
      isAuthenticated(false)
{
    static const bool threads = q_dbus_threads_init_default();
    if (::isDebugging == -1)
       ::isDebugging = qEnvironmentVariableIntValue("QDBUS_DEBUG");
    Q_UNUSED(threads)

#ifdef QDBUS_THREAD_DEBUG
    if (::isDebugging > 1)
        qdbusThreadDebug = qdbusDefaultThreadDebug;
#endif

    QDBusMetaTypeId::init();
    connect(this, &QDBusConnectionPrivate::dispatchStatusChanged,
            this, &QDBusConnectionPrivate::doDispatch, Qt::QueuedConnection);
    connect(this, &QDBusConnectionPrivate::spyHooksFinished,
            this, &QDBusConnectionPrivate::handleObjectCall, Qt::QueuedConnection);
    connect(this, &QDBusConnectionPrivate::messageNeedsSending,
            this, &QDBusConnectionPrivate::sendInternal);
    connect(this, &QDBusConnectionPrivate::signalNeedsConnecting,
            this, &QDBusConnectionPrivate::addSignalHook, Qt::BlockingQueuedConnection);
    connect(this, &QDBusConnectionPrivate::signalNeedsDisconnecting,
            this, &QDBusConnectionPrivate::removeSignalHook, Qt::BlockingQueuedConnection);

    rootNode.flags = 0;

    // prepopulate watchedServices:
    // we know that the owner of org.freedesktop.DBus is itself
    watchedServices.insert(QDBusUtil::dbusService(), WatchedServiceData(QDBusUtil::dbusService(), 1));

    // prepopulate matchRefCounts:
    // we know that org.freedesktop.DBus will never change owners
    matchRefCounts.insert("type='signal',sender='org.freedesktop.DBus',interface='org.freedesktop.DBus',member='NameOwnerChanged',arg0='org.freedesktop.DBus'", 1);
}

QDBusConnectionPrivate::~QDBusConnectionPrivate()
{
    if (thread() && thread() != QThread::currentThread())
        qWarning("QDBusConnection(name=\"%s\")'s last reference in not in its creation thread! "
                 "Timer and socket errors will follow and the program will probably crash",
                 qPrintable(name));

    auto lastMode = mode; // reset on connection close
    closeConnection();
    qDeleteAll(cachedMetaObjects);

    if (lastMode == ClientMode || lastMode == PeerMode) {
        // the bus service object holds a reference back to us;
        // we need to destroy it before we finish destroying ourselves
        Q_ASSERT(ref.loadRelaxed() == 0);
        QObject *obj = (QObject *)busService;
        if (obj) {
            disconnect(obj, nullptr, this, nullptr);
            delete obj;
        }
        if (connection)
            q_dbus_connection_unref(connection);
        connection = nullptr;
    } else if (lastMode == ServerMode) {
        if (server)
            q_dbus_server_unref(server);
        server = nullptr;
    }
}

void QDBusConnectionPrivate::collectAllObjects(QDBusConnectionPrivate::ObjectTreeNode &haystack,
                                               QSet<QObject *> &set)
{
    QDBusConnectionPrivate::ObjectTreeNode::DataList::Iterator it = haystack.children.begin();

    while (it != haystack.children.end()) {
        collectAllObjects(*it, set);
        it++;
    }

    if (haystack.obj)
        set.insert(haystack.obj);
}

void QDBusConnectionPrivate::closeConnection()
{
    QDBusWriteLocker locker(CloseConnectionAction, this);
    qDBusDebug() << this << "Disconnected";
    ConnectionMode oldMode = mode;
    mode = InvalidMode; // prevent reentrancy
    baseService.clear();

    if (oldMode == ServerMode && server) {
        q_dbus_server_disconnect(server);
        q_dbus_server_free_data_slot(&server_slot);
    }

    if (oldMode == ClientMode || oldMode == PeerMode) {
        if (connection) {
            q_dbus_connection_close(connection);
            // send the "close" message
            while (q_dbus_connection_dispatch(connection) == DBUS_DISPATCH_DATA_REMAINS)
                ;
        }
    }

    qDeleteAll(pendingCalls);

    // Disconnect all signals from signal hooks and from the object tree to
    // avoid QObject::destroyed being sent to dbus daemon thread which has
    // already quit. We need to make sure we disconnect exactly once per
    // object, because if we tried a second time, we might be hitting a
    // dangling pointer.
    QSet<QObject *> allObjects;
    collectAllObjects(rootNode, allObjects);
    SignalHookHash::const_iterator sit = signalHooks.constBegin();
    while (sit != signalHooks.constEnd()) {
        allObjects.insert(sit.value().obj);
        ++sit;
    }

    // now disconnect ourselves
    QSet<QObject *>::const_iterator oit = allObjects.constBegin();
    while (oit != allObjects.constEnd()) {
        (*oit)->disconnect(this);
        ++oit;
    }
}

void QDBusConnectionPrivate::handleDBusDisconnection()
{
    while (!pendingCalls.isEmpty())
        processFinishedCall(pendingCalls.first());
}

void QDBusConnectionPrivate::checkThread()
{
    Q_ASSERT(thread() == QDBusConnectionManager::instance());
    Q_ASSERT(QThread::currentThread() == thread());
}

bool QDBusConnectionPrivate::handleError(const QDBusErrorInternal &error)
{
    if (!error)
        return false;           // no error

    //lock.lockForWrite();
    lastError = error;
    //lock.unlock();
    return true;
}

void QDBusConnectionPrivate::timerEvent(QTimerEvent *e)
{
    {
        DBusTimeout *timeout = timeouts.value(e->timerId(), 0);
        if (timeout)
            q_dbus_timeout_handle(timeout);
    }

    doDispatch();
}

void QDBusConnectionPrivate::doDispatch()
{
    if (mode == ClientMode || mode == PeerMode) {
        while (q_dbus_connection_dispatch(connection) == DBUS_DISPATCH_DATA_REMAINS) ;
        if (dispatchEnabled && !pendingMessages.isEmpty()) {
            // dispatch previously queued messages
            PendingMessageList::Iterator it = pendingMessages.begin();
            PendingMessageList::Iterator end = pendingMessages.end();
            for ( ; it != end; ++it) {
                qDBusDebug() << this << "dequeueing message" << *it;
                handleMessage(std::move(*it));
            }
            pendingMessages.clear();
        }
    }
}

void QDBusConnectionPrivate::socketRead(qintptr fd)
{
    WatcherHash::ConstIterator it = watchers.constFind(fd);
    while (it != watchers.constEnd() && it.key() == fd) {
        if (it->watch && it->read && it->read->isEnabled()) {
            if (!q_dbus_watch_handle(it.value().watch, DBUS_WATCH_READABLE))
                qDebug("OUT OF MEM");
            break;
        }
        ++it;
    }
    if ((mode == ClientMode || mode == PeerMode) && !isAuthenticated
        && q_dbus_connection_get_is_authenticated(connection))
        handleAuthentication();
    doDispatch();
}

void QDBusConnectionPrivate::socketWrite(qintptr fd)
{
    WatcherHash::ConstIterator it = watchers.constFind(fd);
    while (it != watchers.constEnd() && it.key() == fd) {
        if (it->watch && it->write && it->write->isEnabled()) {
            if (!q_dbus_watch_handle(it.value().watch, DBUS_WATCH_WRITABLE))
                qDebug("OUT OF MEM");
            break;
        }
        ++it;
    }
    if ((mode == ClientMode || mode == PeerMode) && !isAuthenticated
        && q_dbus_connection_get_is_authenticated(connection))
        handleAuthentication();
}

void QDBusConnectionPrivate::objectDestroyed(QObject *obj)
{
    QDBusWriteLocker locker(ObjectDestroyedAction, this);
    huntAndDestroy(obj, rootNode);

    SignalHookHash::iterator sit = signalHooks.begin();
    while (sit != signalHooks.end()) {
        if (static_cast<QObject *>(sit.value().obj) == obj)
            sit = removeSignalHookNoLock(sit);
        else
            ++sit;
    }

    obj->disconnect(this);
}

void QDBusConnectionPrivate::relaySignal(QObject *obj, const QMetaObject *mo, int signalId,
                                         const QVariantList &args)
{
    QString interface = qDBusInterfaceFromMetaObject(mo);

    QMetaMethod mm = mo->method(signalId);
    QByteArray memberName = mm.name();

    // check if it's scriptable
    bool isScriptable = mm.attributes() & QMetaMethod::Scriptable;
    bool isAdaptor = false;
    for ( ; mo; mo = mo->superClass())
        if (mo == &QDBusAbstractAdaptor::staticMetaObject) {
            isAdaptor = true;
            break;
        }

    checkThread();
    QDBusReadLocker locker(RelaySignalAction, this);
    QDBusMessage message = QDBusMessage::createSignal(QLatin1String("/"), interface,
                                                      QLatin1String(memberName));
    QDBusMessagePrivate::setParametersValidated(message, true);
    message.setArguments(args);
    QDBusError error;
    DBusMessage *msg =
            QDBusMessagePrivate::toDBusMessage(message, connectionCapabilities(), &error);
    if (!msg) {
        qWarning("QDBusConnection: Could not emit signal %s.%s: %s", qPrintable(interface), memberName.constData(),
                 qPrintable(error.message()));
        lastError = error;
        return;
    }

    //qDBusDebug() << "Emitting signal" << message;
    //qDBusDebug() << "for paths:";
    q_dbus_message_set_no_reply(msg, true); // the reply would not be delivered to anything
    huntAndEmit(connection, msg, obj, rootNode, isScriptable, isAdaptor);
    q_dbus_message_unref(msg);
}

void QDBusConnectionPrivate::serviceOwnerChangedNoLock(const QString &name,
                                                       const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(oldOwner);
//    QDBusWriteLocker locker(UpdateSignalHookOwnerAction, this);
    WatchedServicesHash::Iterator it = watchedServices.find(name);
    if (it == watchedServices.end())
        return;
    if (oldOwner != it->owner)
        qWarning("QDBusConnection: name '%s' had owner '%s' but we thought it was '%s'",
                 qPrintable(name), qPrintable(oldOwner), qPrintable(it->owner));

    qDBusDebug() << this << "Updating name" << name << "from" << oldOwner << "to" << newOwner;
    it->owner = newOwner;
}

int QDBusConnectionPrivate::findSlot(QObject* obj, const QByteArray &normalizedName,
                                     QVector<int> &params)
{
    int midx = obj->metaObject()->indexOfMethod(normalizedName);
    if (midx == -1)
        return -1;

    QString errorMsg;
    int inputCount = qDBusParametersForMethod(obj->metaObject()->method(midx), params, errorMsg);
    if ( inputCount == -1 || inputCount + 1 != params.count() )
        return -1;              // failed to parse or invalid arguments or output arguments

    return midx;
}

bool QDBusConnectionPrivate::prepareHook(QDBusConnectionPrivate::SignalHook &hook, QString &key,
                                         const QString &service,
                                         const QString &path, const QString &interface, const QString &name,
                                         const ArgMatchRules &argMatch,
                                         QObject *receiver, const char *signal, int minMIdx,
                                         bool buildSignature)
{
    QByteArray normalizedName = signal + 1;
    hook.midx = findSlot(receiver, signal + 1, hook.params);
    if (hook.midx == -1) {
        normalizedName = QMetaObject::normalizedSignature(signal + 1);
        hook.midx = findSlot(receiver, normalizedName, hook.params);
    }
    if (hook.midx < minMIdx) {
        return false;
    }

    hook.service = service;
    hook.path = path;
    hook.obj = receiver;
    hook.argumentMatch = argMatch;

    // build the D-Bus signal name and signature
    // This should not happen for QDBusConnection::connect, use buildSignature here, since
    // QDBusConnection::connect passes false and everything else uses true
    QString mname = name;
    if (buildSignature && mname.isNull()) {
        normalizedName.truncate(normalizedName.indexOf('('));
        mname = QString::fromUtf8(normalizedName);
    }
    key = mname;
    key.reserve(interface.length() + 1 + mname.length());
    key += QLatin1Char(':');
    key += interface;

    if (buildSignature) {
        hook.signature.clear();
        for (int i = 1; i < hook.params.count(); ++i)
            if (hook.params.at(i) != QDBusMetaTypeId::message())
                hook.signature += QLatin1String( QDBusMetaType::typeToSignature( hook.params.at(i) ) );
    }

    hook.matchRule = buildMatchRule(service, path, interface, mname, argMatch, hook.signature);
    return true;                // connect to this signal
}

void QDBusConnectionPrivate::sendError(const QDBusMessage &msg, QDBusError::ErrorType code)
{
    if (code == QDBusError::UnknownMethod) {
        QString interfaceMsg;
        if (msg.interface().isEmpty())
            interfaceMsg = QLatin1String("any interface");
        else
            interfaceMsg = QLatin1String("interface '%1'").arg(msg.interface());

        send(msg.createErrorReply(code,
                                  QLatin1String("No such method '%1' in %2 at object path '%3' "
                                                "(signature '%4')")
                                  .arg(msg.member(), interfaceMsg, msg.path(), msg.signature())));
    } else if (code == QDBusError::UnknownInterface) {
        send(msg.createErrorReply(QDBusError::UnknownInterface,
                                  QLatin1String("No such interface '%1' at object path '%2'")
                                  .arg(msg.interface(), msg.path())));
    } else if (code == QDBusError::UnknownObject) {
        send(msg.createErrorReply(QDBusError::UnknownObject,
                                  QLatin1String("No such object path '%1'").arg(msg.path())));
    }
}

bool QDBusConnectionPrivate::activateInternalFilters(const ObjectTreeNode &node,
                                                     const QDBusMessage &msg)
{
    // object may be null
    const QString interface = msg.interface();

    if (interface.isEmpty() || interface == QDBusUtil::dbusInterfaceIntrospectable()) {
        if (msg.member() == QLatin1String("Introspect") && msg.signature().isEmpty()) {
            //qDebug() << "QDBusConnectionPrivate::activateInternalFilters introspect" << msg.d_ptr->msg;
            QDBusMessage reply = msg.createReply(qDBusIntrospectObject(node, msg.path()));
            send(reply);
            return true;
        }

        if (!interface.isEmpty()) {
            sendError(msg, QDBusError::UnknownMethod);
            return true;
        }
    }

    if (node.obj && (interface.isEmpty() ||
                     interface == QDBusUtil::dbusInterfaceProperties())) {
        //qDebug() << "QDBusConnectionPrivate::activateInternalFilters properties" << msg.d_ptr->msg;
        if (msg.member() == QLatin1String("Get") && msg.signature() == QLatin1String("ss")) {
            QDBusMessage reply = qDBusPropertyGet(node, msg);
            send(reply);
            return true;
        } else if (msg.member() == QLatin1String("Set") && msg.signature() == QLatin1String("ssv")) {
            QDBusMessage reply = qDBusPropertySet(node, msg);
            send(reply);
            return true;
        } else if (msg.member() == QLatin1String("GetAll") && msg.signature() == QLatin1String("s")) {
            QDBusMessage reply = qDBusPropertyGetAll(node, msg);
            send(reply);
            return true;
        }

        if (!interface.isEmpty()) {
            sendError(msg, QDBusError::UnknownMethod);
            return true;
        }
    }

    return false;
}

void QDBusConnectionPrivate::activateObject(ObjectTreeNode &node, const QDBusMessage &msg,
                                            int pathStartPos)
{
    // This is called by QDBusConnectionPrivate::handleObjectCall to place a call to a slot
    // on the object.
    //
    // The call is routed through the adaptor sub-objects if we have any

    // object may be null

    if (node.flags & QDBusConnectionPrivate::VirtualObject) {
        if (node.treeNode->handleMessage(msg, q(this))) {
            return;
        } else {
            if (activateInternalFilters(node, msg))
                return;
        }
    }

    if (pathStartPos != msg.path().length()) {
        node.flags &= ~QDBusConnection::ExportAllSignals;
        node.obj = findChildObject(&node, msg.path(), pathStartPos);
        if (!node.obj) {
            sendError(msg, QDBusError::UnknownObject);
            return;
        }
    }

    QDBusAdaptorConnector *connector;
    if (node.flags & QDBusConnection::ExportAdaptors &&
        (connector = qDBusFindAdaptorConnector(node.obj))) {
        int newflags = node.flags | QDBusConnection::ExportAllSlots;

        if (msg.interface().isEmpty()) {
            // place the call in all interfaces
            // let the first one that handles it to work
            QDBusAdaptorConnector::AdaptorMap::ConstIterator it =
                connector->adaptors.constBegin();
            QDBusAdaptorConnector::AdaptorMap::ConstIterator end =
                connector->adaptors.constEnd();

            for ( ; it != end; ++it)
                if (activateCall(it->adaptor, newflags, msg))
                    return;
        } else {
            // check if we have an interface matching the name that was asked:
            QDBusAdaptorConnector::AdaptorMap::ConstIterator it;
            it = std::lower_bound(connector->adaptors.constBegin(), connector->adaptors.constEnd(),
                                  msg.interface());
            if (it != connector->adaptors.constEnd() && msg.interface() == QLatin1String(it->interface)) {
                if (!activateCall(it->adaptor, newflags, msg))
                    sendError(msg, QDBusError::UnknownMethod);
                return;
            }
        }
    }

    // no adaptors matched or were exported
    // try our standard filters
    if (activateInternalFilters(node, msg))
        return;                 // internal filters have already run or an error has been sent

    // try the object itself:
    if (node.flags & (QDBusConnection::ExportScriptableSlots|QDBusConnection::ExportNonScriptableSlots) ||
        node.flags & (QDBusConnection::ExportScriptableInvokables|QDBusConnection::ExportNonScriptableInvokables)) {
        bool interfaceFound = true;
        if (!msg.interface().isEmpty()) {
            if (!node.interfaceName.isEmpty())
                interfaceFound = msg.interface() == node.interfaceName;
            else
                interfaceFound = qDBusInterfaceInObject(node.obj, msg.interface());
        }

        if (interfaceFound) {
            if (!activateCall(node.obj, node.flags, msg))
                sendError(msg, QDBusError::UnknownMethod);
            return;
        }
    }

    // nothing matched, send an error code
    if (msg.interface().isEmpty())
        sendError(msg, QDBusError::UnknownMethod);
    else
        sendError(msg, QDBusError::UnknownInterface);
}

void QDBusConnectionPrivate::handleObjectCall(const QDBusMessage &msg)
{
    // if the msg is external, we were called from inside doDispatch
    // that means the dispatchLock mutex is locked
    // must not call out to user code in that case
    //
    // however, if the message is internal, handleMessage was called directly
    // (user's thread) and no lock is in place. We can therefore call out to
    // user code, if necessary.
    ObjectTreeNode result;
    int usedLength;
    QThread *objThread = nullptr;
    QSemaphore sem;
    bool semWait;

    {
        QDBusReadLocker locker(HandleObjectCallAction, this);
        if (!findObject(&rootNode, msg.path(), usedLength, result)) {
            // qDebug("Call failed: no object found at %s", qPrintable(msg.path()));
            sendError(msg, QDBusError::UnknownObject);
            return;
        }

        if (!result.obj) {
            // no object -> no threading issues
            // it's either going to be an error, or an internal filter
            activateObject(result, msg, usedLength);
            return;
        }

        objThread = result.obj->thread();
        if (!objThread) {
            send(msg.createErrorReply(QDBusError::InternalError,
                                      QLatin1String("Object '%1' (at path '%2')"
                                                    " has no thread. Cannot deliver message.")
                                      .arg(result.obj->objectName(), msg.path())));
            return;
        }

        if (!QDBusMessagePrivate::isLocal(msg)) {
            // external incoming message
            // post it and forget
            postEventToThread(HandleObjectCallPostEventAction, result.obj,
                              new QDBusActivateObjectEvent(QDBusConnection(this), this, result,
                                                           usedLength, msg));
            return;
        } else if (objThread != QThread::currentThread()) {
            // looped-back message, targeting another thread:
            // synchronize with it
            postEventToThread(HandleObjectCallPostEventAction, result.obj,
                              new QDBusActivateObjectEvent(QDBusConnection(this), this, result,
                                                           usedLength, msg, &sem));
            semWait = true;
        } else {
            // looped-back message, targeting current thread
            semWait = false;
        }
    } // release the lock

    if (semWait)
        SEM_ACQUIRE(HandleObjectCallSemaphoreAction, sem);
    else
        activateObject(result, msg, usedLength);
}

QDBusActivateObjectEvent::~QDBusActivateObjectEvent()
{
    if (!handled) {
        // we're being destroyed without delivering
        // it means the object was deleted between posting and delivering
        QDBusConnectionPrivate *that = QDBusConnectionPrivate::d(connection);
        that->sendError(message, QDBusError::UnknownObject);
    }

    // semaphore releasing happens in ~QMetaCallEvent
}

void QDBusActivateObjectEvent::placeMetaCall(QObject *)
{
    QDBusConnectionPrivate *that = QDBusConnectionPrivate::d(connection);

    QDBusLockerBase::reportThreadAction(HandleObjectCallPostEventAction,
                                        QDBusLockerBase::BeforeDeliver, that);
    that->activateObject(node, message, pathStartPos);
    QDBusLockerBase::reportThreadAction(HandleObjectCallPostEventAction,
                                        QDBusLockerBase::AfterDeliver, that);

    handled = true;
}

void QDBusConnectionPrivate::handleSignal(const QString &key, const QDBusMessage& msg)
{
    SignalHookHash::const_iterator it = signalHooks.constFind(key);
    SignalHookHash::const_iterator end = signalHooks.constEnd();
    //qDebug("looking for: %s", path.toLocal8Bit().constData());
    //qDBusDebug() << signalHooks.keys();
    for ( ; it != end && it.key() == key; ++it) {
        const SignalHook &hook = it.value();
        if (!hook.service.isEmpty()) {
            QString owner = watchedServices.value(hook.service, WatchedServiceData(hook.service)).owner;
            if (owner != msg.service())
                continue;
        }
        if (!hook.path.isEmpty() && hook.path != msg.path())
            continue;
        if (!hook.signature.isEmpty() && hook.signature != msg.signature())
            continue;
        if (hook.signature.isEmpty() && !hook.signature.isNull() && !msg.signature().isEmpty())
            continue;
        if (!hook.argumentMatch.args.isEmpty()) {
            const QVariantList arguments = msg.arguments();
            if (hook.argumentMatch.args.size() > arguments.size())
                continue;

            bool matched = true;
            for (int i = 0; i < hook.argumentMatch.args.size(); ++i) {
                const QString &param = hook.argumentMatch.args.at(i);
                if (param.isNull())
                    continue;   // don't try to match against this
                if (param == arguments.at(i).toString())
                    continue;   // matched
                matched = false;
                break;
            }
            if (!matched)
                continue;
        }
        if (!hook.argumentMatch.arg0namespace.isEmpty()) {
            const QVariantList arguments = msg.arguments();
            if (arguments.size() < 1)
                continue;
            const QString param = arguments.at(0).toString();
            if (param != hook.argumentMatch.arg0namespace
                && !param.startsWith(hook.argumentMatch.arg0namespace + QLatin1Char('.')))
                continue;
        }
        activateSignal(hook, msg);
    }
}

void QDBusConnectionPrivate::handleSignal(const QDBusMessage& msg)
{
    // We call handlesignal(QString, QDBusMessage) three times:
    //  one with member:interface
    //  one with member:
    //  one with :interface
    // This allows us to match signals with wildcards on member or interface
    // (but not both)

    QString key = msg.member();
    key.reserve(key.length() + 1 + msg.interface().length());
    key += QLatin1Char(':');
    key += msg.interface();

    QDBusReadLocker locker(HandleSignalAction, this);
    handleSignal(key, msg);                  // one try

    key.truncate(msg.member().length() + 1); // keep the ':'
    handleSignal(key, msg);                  // second try

    key = QLatin1Char(':');
    key += msg.interface();
    handleSignal(key, msg);                  // third try
}

void QDBusConnectionPrivate::watchForDBusDisconnection()
{
    SignalHook hook;
    // Initialize the hook for Disconnected signal
    hook.service.clear(); // org.freedesktop.DBus.Local.Disconnected uses empty service name
    hook.path = QDBusUtil::dbusPathLocal();
    hook.obj = this;
    hook.params << QMetaType::Void;
    hook.midx = staticMetaObject.indexOfSlot("handleDBusDisconnection()");
    Q_ASSERT(hook.midx != -1);
    signalHooks.insert(QLatin1String("Disconnected:" DBUS_INTERFACE_LOCAL), hook);
}

void QDBusConnectionPrivate::setServer(QDBusServer *object, DBusServer *s, const QDBusErrorInternal &error)
{
    mode = ServerMode;
    serverObject = object;
    object->d = this;
    if (!s) {
        handleError(error);
        return;
    }

    server = s;

    dbus_bool_t data_allocated = q_dbus_server_allocate_data_slot(&server_slot);
    if (data_allocated && server_slot < 0)
        return;

    dbus_bool_t watch_functions_set = q_dbus_server_set_watch_functions(server,
                                                                      qDBusAddWatch,
                                                                      qDBusRemoveWatch,
                                                                      qDBusToggleWatch,
                                                                      this, nullptr);
    //qDebug() << "watch_functions_set" << watch_functions_set;
    Q_UNUSED(watch_functions_set);

    dbus_bool_t time_functions_set = q_dbus_server_set_timeout_functions(server,
                                                                       qDBusAddTimeout,
                                                                       qDBusRemoveTimeout,
                                                                       qDBusToggleTimeout,
                                                                       this, nullptr);
    //qDebug() << "time_functions_set" << time_functions_set;
    Q_UNUSED(time_functions_set);

    q_dbus_server_set_new_connection_function(server, qDBusNewConnection, this, nullptr);

    dbus_bool_t data_set = q_dbus_server_set_data(server, server_slot, this, nullptr);
    //qDebug() << "data_set" << data_set;
    Q_UNUSED(data_set);
}

void QDBusConnectionPrivate::setPeer(DBusConnection *c, const QDBusErrorInternal &error)
{
    mode = PeerMode;
    if (!c) {
        handleError(error);
        return;
    }

    connection = c;

    q_dbus_connection_set_exit_on_disconnect(connection, false);
    q_dbus_connection_set_watch_functions(connection,
                                        qDBusAddWatch,
                                        qDBusRemoveWatch,
                                        qDBusToggleWatch,
                                        this, nullptr);
    q_dbus_connection_set_timeout_functions(connection,
                                          qDBusAddTimeout,
                                          qDBusRemoveTimeout,
                                          qDBusToggleTimeout,
                                          this, nullptr);
    q_dbus_connection_set_dispatch_status_function(connection, qDBusUpdateDispatchStatus, this, nullptr);
    q_dbus_connection_add_filter(connection,
                               qDBusSignalFilter,
                               this, nullptr);

    watchForDBusDisconnection();

    QMetaObject::invokeMethod(this, "doDispatch", Qt::QueuedConnection);
}

static QDBusConnection::ConnectionCapabilities connectionCapabilies(DBusConnection *connection)
{
    QDBusConnection::ConnectionCapabilities result;
    typedef dbus_bool_t (*can_send_type_t)(DBusConnection *, int);
    static can_send_type_t can_send_type = nullptr;

#if defined(QT_LINKED_LIBDBUS)
# if DBUS_VERSION-0 >= 0x010400
    can_send_type = dbus_connection_can_send_type;
# endif
#elif QT_CONFIG(library)
    // run-time check if the next functions are available
    can_send_type = (can_send_type_t)qdbus_resolve_conditionally("dbus_connection_can_send_type");
#endif

#ifndef DBUS_TYPE_UNIX_FD
# define DBUS_TYPE_UNIX_FD int('h')
#endif
    if (can_send_type && can_send_type(connection, DBUS_TYPE_UNIX_FD))
        result |= QDBusConnection::UnixFileDescriptorPassing;

    return result;
}

void QDBusConnectionPrivate::handleAuthentication()
{
    capabilities.storeRelaxed(connectionCapabilies(connection));
    isAuthenticated = true;
}

void QDBusConnectionPrivate::setConnection(DBusConnection *dbc, const QDBusErrorInternal &error)
{
    mode = ClientMode;
    if (!dbc) {
        handleError(error);
        return;
    }

    connection = dbc;

    const char *service = q_dbus_bus_get_unique_name(connection);
    Q_ASSERT(service);
    baseService = QString::fromUtf8(service);
    // bus connections are already authenticated here because q_dbus_bus_register() has been called
    handleAuthentication();

    q_dbus_connection_set_exit_on_disconnect(connection, false);
    q_dbus_connection_set_watch_functions(connection, qDBusAddWatch, qDBusRemoveWatch,
                                          qDBusToggleWatch, this, nullptr);
    q_dbus_connection_set_timeout_functions(connection, qDBusAddTimeout, qDBusRemoveTimeout,
                                            qDBusToggleTimeout, this, nullptr);
    q_dbus_connection_set_dispatch_status_function(connection, qDBusUpdateDispatchStatus, this, nullptr);
    q_dbus_connection_add_filter(connection, qDBusSignalFilter, this, nullptr);

    // Initialize the hooks for the NameAcquired and NameLost signals
    // we don't use connectSignal here because we don't need the rules to be sent to the bus
    // the bus will always send us these two signals
    SignalHook hook;
    hook.service = QDBusUtil::dbusService();
    hook.path.clear(); // no matching
    hook.obj = this;
    hook.params << QMetaType::Void << QMetaType::QString; // both functions take a QString as parameter and return void

    hook.midx = staticMetaObject.indexOfSlot("registerServiceNoLock(QString)");
    Q_ASSERT(hook.midx != -1);
    signalHooks.insert(QLatin1String("NameAcquired:" DBUS_INTERFACE_DBUS), hook);

    hook.midx = staticMetaObject.indexOfSlot("unregisterServiceNoLock(QString)");
    Q_ASSERT(hook.midx != -1);
    signalHooks.insert(QLatin1String("NameLost:" DBUS_INTERFACE_DBUS), hook);

    // And initialize the hook for the NameOwnerChanged signal;
    // we don't use connectSignal here because the rules are added by connectSignal on a per-need basis
    hook.params.clear();
    hook.params.reserve(4);
    hook.params << QMetaType::Void << QMetaType::QString << QMetaType::QString << QMetaType::QString;
    hook.midx = staticMetaObject.indexOfSlot("serviceOwnerChangedNoLock(QString,QString,QString)");
    Q_ASSERT(hook.midx != -1);
    signalHooks.insert(QLatin1String("NameOwnerChanged:" DBUS_INTERFACE_DBUS), hook);

    watchForDBusDisconnection();

    qDBusDebug() << this << ": connected successfully";

    // schedule a dispatch:
    QMetaObject::invokeMethod(this, "doDispatch", Qt::QueuedConnection);
}

extern "C"{
static void qDBusResultReceived(DBusPendingCall *pending, void *user_data)
{
    QDBusPendingCallPrivate *call = reinterpret_cast<QDBusPendingCallPrivate *>(user_data);
    Q_ASSERT(call->pending == pending);
    Q_UNUSED(pending);
    QDBusConnectionPrivate::processFinishedCall(call);
}
}

void QDBusConnectionPrivate::processFinishedCall(QDBusPendingCallPrivate *call)
{
    QDBusConnectionPrivate *connection = const_cast<QDBusConnectionPrivate *>(call->connection);

    auto locker = qt_unique_lock(call->mutex);

    connection->pendingCalls.removeOne(call);

    QDBusMessage &msg = call->replyMessage;
    if (call->pending) {
        // when processFinishedCall is called and pending call is not completed,
        // it means we received disconnected signal from libdbus
        if (q_dbus_pending_call_get_completed(call->pending)) {
            // decode the message
            DBusMessage *reply = q_dbus_pending_call_steal_reply(call->pending);
            msg = QDBusMessagePrivate::fromDBusMessage(reply, connection->connectionCapabilities());
            q_dbus_message_unref(reply);
        } else {
            msg = QDBusMessage::createError(QDBusError::Disconnected, QDBusUtil::disconnectedErrorMessage());
        }
    }
    qDBusDebug() << connection << "got message reply:" << msg;

    // Check if the reply has the expected signature
    call->checkReceivedSignature();

    if (!call->receiver.isNull() && call->methodIdx != -1 && msg.type() == QDBusMessage::ReplyMessage) {
        // Deliver the return values of a remote function call.
        //
        // There is only one connection and it is specified by idx
        // The slot must have the same parameter types that the message does
        // The slot may have less parameters than the message
        // The slot may optionally have one final parameter that is QDBusMessage
        // The slot receives read-only copies of the message (i.e., pass by value or by const-ref)

        QDBusCallDeliveryEvent *e = prepareReply(connection, call->receiver, call->methodIdx,
                                                 call->metaTypes, msg);
        if (e)
            connection->postEventToThread(MessageResultReceivedAction, call->receiver, e);
        else
            qDBusDebug("Deliver failed!");
    }

    if (call->pending) {
        q_dbus_pending_call_unref(call->pending);
        call->pending = nullptr;
    }

    // Are there any watchers?
    if (call->watcherHelper)
        call->watcherHelper->emitSignals(msg, call->sentMessage);

    call->waitForFinishedCondition.wakeAll();
    locker.unlock();

    if (msg.type() == QDBusMessage::ErrorMessage)
        emit connection->callWithCallbackFailed(QDBusError(msg), call->sentMessage);

    if (!call->ref.deref())
        delete call;
}

bool QDBusConnectionPrivate::send(const QDBusMessage& message)
{
    if (QDBusMessagePrivate::isLocal(message))
        return true;            // don't send; the reply will be retrieved by the caller
                                // through the d_ptr->localReply link

    QDBusError error;
    DBusMessage *msg =
            QDBusMessagePrivate::toDBusMessage(message, connectionCapabilities(), &error);
    if (!msg) {
        if (message.type() == QDBusMessage::MethodCallMessage)
            qWarning("QDBusConnection: error: could not send message to service \"%s\" path \"%s\" interface \"%s\" member \"%s\": %s",
                     qPrintable(message.service()), qPrintable(message.path()),
                     qPrintable(message.interface()), qPrintable(message.member()),
                     qPrintable(error.message()));
        else if (message.type() == QDBusMessage::SignalMessage)
            qWarning("QDBusConnection: error: could not send signal to service \"%s\" path \"%s\" interface \"%s\" member \"%s\": %s",
                     qPrintable(message.service()),
                     qPrintable(message.path()), qPrintable(message.interface()),
                     qPrintable(message.member()),
                     qPrintable(error.message()));
        else
            qWarning("QDBusConnection: error: could not send %s message to service \"%s\": %s",
                     message.type() == QDBusMessage::ReplyMessage ? "reply" :
                     message.type() == QDBusMessage::ErrorMessage ? "error" :
                     "invalid", qPrintable(message.service()),
                     qPrintable(error.message()));
        lastError = error;
        return false;
    }

    q_dbus_message_set_no_reply(msg, true); // the reply would not be delivered to anything
    qDBusDebug() << this << "sending message (no reply):" << message;
    emit messageNeedsSending(nullptr, msg);
    return true;
}

// small helper to note long running blocking dbus calls.
// these are generally a sign of fragile software (too long a call can either
// lead to bad user experience, if it's running on the GUI thread for instance)
// or break completely under load (hitting the call timeout).
//
// as a result, this is something we want to watch for.
class QDBusBlockingCallWatcher
{
public:
    QDBusBlockingCallWatcher(const QDBusMessage &message)
        : m_message(message), m_maxCallTimeoutMs(0)
    {
#if defined(QT_NO_DEBUG)
        // when in a release build, we default these to off.
        // this means that we only affect code that explicitly enables the warning.
        static int mainThreadWarningAmount = -1;
        static int otherThreadWarningAmount = -1;
#else
        static int mainThreadWarningAmount = 200;
        static int otherThreadWarningAmount = 500;
#endif
        static bool initializedAmounts = false;
        static QBasicMutex initializeMutex;
        auto locker = qt_unique_lock(initializeMutex);

        if (!initializedAmounts) {
            int tmp = 0;
            QByteArray env;
            bool ok = true;

            env = qgetenv("Q_DBUS_BLOCKING_CALL_MAIN_THREAD_WARNING_MS");
            if (!env.isEmpty()) {
                tmp = env.toInt(&ok);
                if (ok)
                    mainThreadWarningAmount = tmp;
                else
                    qWarning("QDBusBlockingCallWatcher: Q_DBUS_BLOCKING_CALL_MAIN_THREAD_WARNING_MS must be an integer; value ignored");
            }

            env = qgetenv("Q_DBUS_BLOCKING_CALL_OTHER_THREAD_WARNING_MS");
            if (!env.isEmpty()) {
                tmp = env.toInt(&ok);
                if (ok)
                    otherThreadWarningAmount = tmp;
                else
                    qWarning("QDBusBlockingCallWatcher: Q_DBUS_BLOCKING_CALL_OTHER_THREAD_WARNING_MS must be an integer; value ignored");
            }

            initializedAmounts = true;
        }

        locker.unlock();

        // if this call is running on the main thread, we have a much lower
        // tolerance for delay because any long-term delay will wreck user
        // interactivity.
        if (qApp && qApp->thread() == QThread::currentThread())
            m_maxCallTimeoutMs = mainThreadWarningAmount;
        else
            m_maxCallTimeoutMs = otherThreadWarningAmount;

        m_callTimer.start();
    }

    ~QDBusBlockingCallWatcher()
    {
        if (m_maxCallTimeoutMs < 0)
            return; // disabled

        if (m_callTimer.elapsed() >= m_maxCallTimeoutMs) {
            qWarning("QDBusConnection: warning: blocking call took a long time (%d ms, max for this thread is %d ms) to service \"%s\" path \"%s\" interface \"%s\" member \"%s\"",
                     int(m_callTimer.elapsed()), m_maxCallTimeoutMs,
                     qPrintable(m_message.service()), qPrintable(m_message.path()),
                     qPrintable(m_message.interface()), qPrintable(m_message.member()));
        }
    }

private:
    QDBusMessage m_message;
    int m_maxCallTimeoutMs;
    QElapsedTimer m_callTimer;
};


QDBusMessage QDBusConnectionPrivate::sendWithReply(const QDBusMessage &message,
                                                   int sendMode, int timeout)
{
    QDBusBlockingCallWatcher watcher(message);

    QDBusPendingCallPrivate *pcall = sendWithReplyAsync(message, nullptr, nullptr, nullptr, timeout);
    Q_ASSERT(pcall);

    if (pcall->replyMessage.type() == QDBusMessage::InvalidMessage) {
        // need to wait for the reply
        if (sendMode == QDBus::BlockWithGui) {
            pcall->watcherHelper = new QDBusPendingCallWatcherHelper;
            QEventLoop loop;
            loop.connect(pcall->watcherHelper, &QDBusPendingCallWatcherHelper::reply, &loop, &QEventLoop::quit);
            loop.connect(pcall->watcherHelper, &QDBusPendingCallWatcherHelper::error, &loop, &QEventLoop::quit);

            // enter the event loop and wait for a reply
            loop.exec(QEventLoop::ExcludeUserInputEvents | QEventLoop::WaitForMoreEvents);
        } else {
            pcall->waitForFinished();
        }
    }

    QDBusMessage reply = pcall->replyMessage;
    lastError = QDBusError(reply);      // set or clear error

    if (!pcall->ref.deref())
        delete pcall;
    return reply;
}

QDBusMessage QDBusConnectionPrivate::sendWithReplyLocal(const QDBusMessage &message)
{
    qDBusDebug() << this << "sending message via local-loop:" << message;

    QDBusMessage localCallMsg = QDBusMessagePrivate::makeLocal(*this, message);
    bool handled = handleMessage(localCallMsg);

    if (!handled) {
        QString interface = message.interface();
        if (interface.isEmpty())
            interface = QLatin1String("<no-interface>");
        return QDBusMessage::createError(QDBusError::InternalError,
                                         QLatin1String("Internal error trying to call %1.%2 at %3 (signature '%4'")
                                         .arg(interface, message.member(),
                                              message.path(), message.signature()));
    }

    // if the message was handled, there might be a reply
    QDBusMessage localReplyMsg = QDBusMessagePrivate::makeLocalReply(*this, localCallMsg);
    if (localReplyMsg.type() == QDBusMessage::InvalidMessage) {
        qWarning("QDBusConnection: cannot call local method '%s' at object %s (with signature '%s') "
                 "on blocking mode", qPrintable(message.member()), qPrintable(message.path()),
                 qPrintable(message.signature()));
        return QDBusMessage::createError(
            QDBusError(QDBusError::InternalError,
                       QLatin1String("local-loop message cannot have delayed replies")));
    }

    // there is a reply
    qDBusDebug() << this << "got message via local-loop:" << localReplyMsg;
    return localReplyMsg;
}

QDBusPendingCallPrivate *QDBusConnectionPrivate::sendWithReplyAsync(const QDBusMessage &message,
                                                                    QObject *receiver, const char *returnMethod,
                                                                    const char *errorMethod, int timeout)
{
    QDBusPendingCallPrivate *pcall = new QDBusPendingCallPrivate(message, this);
    bool isLoopback;
    if ((isLoopback = isServiceRegisteredByThread(message.service()))) {
        // special case for local calls
        pcall->replyMessage = sendWithReplyLocal(message);
    }

    if (receiver && returnMethod)
        pcall->setReplyCallback(receiver, returnMethod);

    if (errorMethod) {
        pcall->watcherHelper = new QDBusPendingCallWatcherHelper;
        connect(pcall->watcherHelper, SIGNAL(error(QDBusError,QDBusMessage)), receiver, errorMethod,
                Qt::QueuedConnection);
        pcall->watcherHelper->moveToThread(thread());
    }

    if ((receiver && returnMethod) || errorMethod) {
       // no one waiting, will delete pcall in processFinishedCall()
       pcall->ref.storeRelaxed(1);
    } else {
       // set double ref to prevent race between processFinishedCall() and ref counting
       // by QDBusPendingCall::QExplicitlySharedDataPointer<QDBusPendingCallPrivate>
       pcall->ref.storeRelaxed(2);
    }

    if (isLoopback) {
        // a loopback call
        processFinishedCall(pcall);
        return pcall;
    }

    QDBusError error;
    DBusMessage *msg =
            QDBusMessagePrivate::toDBusMessage(message, connectionCapabilities(), &error);
    if (!msg) {
        qWarning("QDBusConnection: error: could not send message to service \"%s\" path \"%s\" interface \"%s\" member \"%s\": %s",
                 qPrintable(message.service()), qPrintable(message.path()),
                 qPrintable(message.interface()), qPrintable(message.member()),
                 qPrintable(error.message()));
        pcall->replyMessage = QDBusMessage::createError(error);
        lastError = error;
        processFinishedCall(pcall);
    } else {
        qDBusDebug() << this << "sending message:" << message;
        emit messageNeedsSending(pcall, msg, timeout);
    }
    return pcall;
}

void QDBusConnectionPrivate::sendInternal(QDBusPendingCallPrivate *pcall, void *message, int timeout)
{
    QDBusError error;
    DBusPendingCall *pending = nullptr;
    DBusMessage *msg = static_cast<DBusMessage *>(message);
    bool isNoReply = !pcall;
    Q_ASSERT(isNoReply == !!q_dbus_message_get_no_reply(msg));

    checkThread();

    if (isNoReply && q_dbus_connection_send(connection, msg, nullptr)) {
        // success
    } else if (!isNoReply && q_dbus_connection_send_with_reply(connection, msg, &pending, timeout)) {
        if (pending) {
            q_dbus_message_unref(msg);

            pcall->pending = pending;
            q_dbus_pending_call_set_notify(pending, qDBusResultReceived, pcall, nullptr);

            // DBus won't notify us when a peer disconnects or server terminates so we need to track these ourselves
            if (mode == QDBusConnectionPrivate::PeerMode || mode == QDBusConnectionPrivate::ClientMode)
                pendingCalls.append(pcall);

            return;
        } else {
            // we're probably disconnected at this point
            lastError = error = QDBusError(QDBusError::Disconnected, QDBusUtil::disconnectedErrorMessage());
        }
    } else {
        lastError = error = QDBusError(QDBusError::NoMemory, QStringLiteral("Out of memory"));
    }

    q_dbus_message_unref(msg);
    if (pcall) {
        pcall->replyMessage = QDBusMessage::createError(error);
        processFinishedCall(pcall);
    }
}


bool QDBusConnectionPrivate::connectSignal(const QString &service,
                                           const QString &path, const QString &interface, const QString &name,
                                           const QStringList &argumentMatch, const QString &signature,
                                           QObject *receiver, const char *slot)
{
    ArgMatchRules rules;
    rules.args = argumentMatch;
    return connectSignal(service, path, interface, name, rules, signature, receiver, slot);
}

bool QDBusConnectionPrivate::connectSignal(const QString &service,
                                           const QString &path, const QString &interface, const QString &name,
                                           const ArgMatchRules &argumentMatch, const QString &signature,
                                           QObject *receiver, const char *slot)
{
    // check the slot
    QDBusConnectionPrivate::SignalHook hook;
    QString key;

    hook.signature = signature;
    if (!prepareHook(hook, key, service, path, interface, name, argumentMatch, receiver, slot, 0, false))
        return false;           // don't connect

    Q_ASSERT(thread() != QThread::currentThread());
    return emit signalNeedsConnecting(key, hook);
}

bool QDBusConnectionPrivate::addSignalHook(const QString &key, const SignalHook &hook)
{
    QDBusWriteLocker locker(ConnectAction, this);

    // avoid duplicating:
    QDBusConnectionPrivate::SignalHookHash::ConstIterator it = signalHooks.constFind(key);
    QDBusConnectionPrivate::SignalHookHash::ConstIterator end = signalHooks.constEnd();
    for ( ; it != end && it.key() == key; ++it) {
        const QDBusConnectionPrivate::SignalHook &entry = it.value();
        if (entry.service == hook.service &&
            entry.path == hook.path &&
            entry.signature == hook.signature &&
            entry.obj == hook.obj &&
            entry.midx == hook.midx &&
            entry.argumentMatch == hook.argumentMatch) {
            // no need to compare the parameters if it's the same slot
            return false;     // already there
        }
    }

    signalHooks.insert(key, hook);
    connect(hook.obj, &QObject::destroyed, this, &QDBusConnectionPrivate::objectDestroyed,
            Qt::ConnectionType(Qt::BlockingQueuedConnection | Qt::UniqueConnection));

    MatchRefCountHash::iterator mit = matchRefCounts.find(hook.matchRule);

    if (mit != matchRefCounts.end()) { // Match already present
        mit.value() = mit.value() + 1;
        return true;
    }

    matchRefCounts.insert(hook.matchRule, 1);

    if (connection) {
        if (mode != QDBusConnectionPrivate::PeerMode) {
            qDBusDebug() << this << "Adding rule:" << hook.matchRule;
            q_dbus_bus_add_match(connection, hook.matchRule, nullptr);

            // Successfully connected the signal
            // Do we need to watch for this name?
            if (shouldWatchService(hook.service)) {
                WatchedServicesHash::mapped_type &data = watchedServices[hook.service];
                if (++data.refcount == 1) {
                    // we need to watch for this service changing
                    ArgMatchRules rules;
                    rules.args << hook.service;
                    q_dbus_bus_add_match(connection,
                                         buildMatchRule(QDBusUtil::dbusService(), QString(), QDBusUtil::dbusInterface(),
                                                        QDBusUtil::nameOwnerChanged(), rules, QString()),
                                         nullptr);
                    data.owner = getNameOwnerNoCache(hook.service);
                    qDBusDebug() << this << "Watching service" << hook.service << "for owner changes (current owner:"
                                 << data.owner << ")";
                }
            }
        }
    }
    return true;
}

bool QDBusConnectionPrivate::disconnectSignal(const QString &service,
                                           const QString &path, const QString &interface, const QString &name,
                                           const QStringList &argumentMatch, const QString &signature,
                                           QObject *receiver, const char *slot)
{
    ArgMatchRules rules;
    rules.args = argumentMatch;
    return disconnectSignal(service, path, interface, name, rules, signature, receiver, slot);
}

bool QDBusConnectionPrivate::disconnectSignal(const QString &service,
                                              const QString &path, const QString &interface, const QString &name,
                                              const ArgMatchRules &argumentMatch, const QString &signature,
                                              QObject *receiver, const char *slot)
{
    // check the slot
    QDBusConnectionPrivate::SignalHook hook;
    QString key;
    QString name2 = name;
    if (name2.isNull())
        name2.detach();

    hook.signature = signature;
    if (!prepareHook(hook, key, service, path, interface, name, argumentMatch, receiver, slot, 0, false))
        return false;           // don't disconnect

    Q_ASSERT(thread() != QThread::currentThread());
    return emit signalNeedsDisconnecting(key, hook);
}

bool QDBusConnectionPrivate::removeSignalHook(const QString &key, const SignalHook &hook)
{
    // remove it from our list:
    QDBusWriteLocker locker(ConnectAction, this);
    QDBusConnectionPrivate::SignalHookHash::Iterator it = signalHooks.find(key);
    QDBusConnectionPrivate::SignalHookHash::Iterator end = signalHooks.end();
    for ( ; it != end && it.key() == key; ++it) {
        const QDBusConnectionPrivate::SignalHook &entry = it.value();
        if (entry.service == hook.service &&
            entry.path == hook.path &&
            entry.signature == hook.signature &&
            entry.obj == hook.obj &&
            entry.midx == hook.midx &&
            entry.argumentMatch.args == hook.argumentMatch.args) {
            // no need to compare the parameters if it's the same slot
            removeSignalHookNoLock(it);
            return true;        // it was there
        }
    }

    // the slot was not found
    return false;
}

QDBusConnectionPrivate::SignalHookHash::Iterator
QDBusConnectionPrivate::removeSignalHookNoLock(SignalHookHash::Iterator it)
{
    const SignalHook &hook = it.value();

    bool erase = false;
    MatchRefCountHash::iterator i = matchRefCounts.find(hook.matchRule);
    if (i == matchRefCounts.end()) {
        qWarning("QDBusConnectionPrivate::disconnectSignal: MatchRule not found in matchRefCounts!!");
    } else {
        if (i.value() == 1) {
            erase = true;
            matchRefCounts.erase(i);
        }
        else {
            i.value() = i.value() - 1;
        }
    }

    // we don't care about errors here
    if (connection && erase) {
        if (mode != QDBusConnectionPrivate::PeerMode) {
            qDBusDebug() << this << "Removing rule:" << hook.matchRule;
            q_dbus_bus_remove_match(connection, hook.matchRule, nullptr);

            // Successfully disconnected the signal
            // Were we watching for this name?
            WatchedServicesHash::Iterator sit = watchedServices.find(hook.service);
            if (sit != watchedServices.end()) {
                if (--sit.value().refcount == 0) {
                    watchedServices.erase(sit);
                    ArgMatchRules rules;
                    rules.args << hook.service;
                    q_dbus_bus_remove_match(connection,
                                            buildMatchRule(QDBusUtil::dbusService(), QString(), QDBusUtil::dbusInterface(),
                                                           QDBusUtil::nameOwnerChanged(), rules, QString()),
                                            nullptr);
                }
            }
        }

    }

    return signalHooks.erase(it);
}

void QDBusConnectionPrivate::registerObject(const ObjectTreeNode *node)
{
    connect(node->obj, &QObject::destroyed, this, &QDBusConnectionPrivate::objectDestroyed,
            Qt::ConnectionType(Qt::BlockingQueuedConnection | Qt::UniqueConnection));

    if (node->flags & (QDBusConnection::ExportAdaptors
                       | QDBusConnection::ExportScriptableSignals
                       | QDBusConnection::ExportNonScriptableSignals)) {
        QDBusAdaptorConnector *connector = qDBusCreateAdaptorConnector(node->obj);

        if (node->flags & (QDBusConnection::ExportScriptableSignals
                           | QDBusConnection::ExportNonScriptableSignals)) {
            connector->disconnectAllSignals(node->obj);
            connector->connectAllSignals(node->obj);
        }

        connect(connector, SIGNAL(relaySignal(QObject*,const QMetaObject*,int,QVariantList)),
                this, SLOT(relaySignal(QObject*,const QMetaObject*,int,QVariantList)),
                Qt::ConnectionType(Qt::QueuedConnection | Qt::UniqueConnection));
    }
}

void QDBusConnectionPrivate::unregisterObject(const QString &path, QDBusConnection::UnregisterMode mode)
{
    QDBusConnectionPrivate::ObjectTreeNode *node = &rootNode;
    QVector<QStringRef> pathComponents;
    int i;
    if (path == QLatin1String("/")) {
        i = 0;
    } else {
        pathComponents = path.splitRef(QLatin1Char('/'));
        i = 1;
    }

    huntAndUnregister(pathComponents, i, mode, node);
}

void QDBusConnectionPrivate::connectRelay(const QString &service,
                                          const QString &path, const QString &interface,
                                          QDBusAbstractInterface *receiver,
                                          const QMetaMethod &signal)
{
    // this function is called by QDBusAbstractInterface when one of its signals is connected
    // we set up a relay from D-Bus into it
    SignalHook hook;
    QString key;

    QByteArray sig;
    sig.append(QSIGNAL_CODE + '0');
    sig.append(signal.methodSignature());
    if (!prepareHook(hook, key, service, path, interface, QString(), ArgMatchRules(), receiver, sig,
                     QDBusAbstractInterface::staticMetaObject.methodCount(), true))
        return;                 // don't connect

    Q_ASSERT(thread() != QThread::currentThread());
    emit signalNeedsConnecting(key, hook);
}

void QDBusConnectionPrivate::disconnectRelay(const QString &service,
                                             const QString &path, const QString &interface,
                                             QDBusAbstractInterface *receiver,
                                             const QMetaMethod &signal)
{
    // this function is called by QDBusAbstractInterface when one of its signals is disconnected
    // we remove relay from D-Bus into it
    SignalHook hook;
    QString key;

    QByteArray sig;
    sig.append(QSIGNAL_CODE + '0');
    sig.append(signal.methodSignature());
    if (!prepareHook(hook, key, service, path, interface, QString(), ArgMatchRules(), receiver, sig,
                     QDBusAbstractInterface::staticMetaObject.methodCount(), true))
        return;                 // don't disconnect

    Q_ASSERT(thread() != QThread::currentThread());
    emit signalNeedsDisconnecting(key, hook);
}

bool QDBusConnectionPrivate::shouldWatchService(const QString &service)
{
    // we don't have to watch anything in peer mode
    if (mode != ClientMode)
        return false;
    // we don't have to watch wildcard services (empty strings)
    if (service.isEmpty())
        return false;
    // we don't have to watch the bus driver
    if (service == QDBusUtil::dbusService())
        return false;
    return true;
}

/*!
    Sets up a watch rule for service \a service for the change described by
    mode \a mode. When the change happens, slot \a member in object \a obj will
    be called.

    The caller should call QDBusConnectionPrivate::shouldWatchService() before
    calling this function to check whether the service needs to be watched at
    all. Failing to do so may add rules that are never activated.
*/
void QDBusConnectionPrivate::watchService(const QString &service, QDBusServiceWatcher::WatchMode mode, QObject *obj, const char *member)
{
    ArgMatchRules matchArgs = matchArgsForService(service, mode);
    connectSignal(QDBusUtil::dbusService(), QString(), QDBusUtil::dbusInterface(), QDBusUtil::nameOwnerChanged(),
                  matchArgs, QString(), obj, member);
}

/*!
    Removes a watch rule set up by QDBusConnectionPrivate::watchService(). The
    arguments to this function must be the same as the ones for that function.

    Sets up a watch rule for service \a service for the change described by
    mode \a mode. When the change happens, slot \a member in object \a obj will
    be called.
*/
void QDBusConnectionPrivate::unwatchService(const QString &service, QDBusServiceWatcher::WatchMode mode, QObject *obj, const char *member)
{
    ArgMatchRules matchArgs = matchArgsForService(service, mode);
    disconnectSignal(QDBusUtil::dbusService(), QString(), QDBusUtil::dbusInterface(), QDBusUtil::nameOwnerChanged(),
                     matchArgs, QString(), obj, member);
}

QString QDBusConnectionPrivate::getNameOwner(const QString& serviceName)
{
    if (QDBusUtil::isValidUniqueConnectionName(serviceName))
        return serviceName;
    if (!connection)
        return QString();

    {
        // acquire a read lock for the cache
        QReadLocker locker(&lock);
        WatchedServicesHash::ConstIterator it = watchedServices.constFind(serviceName);
        if (it != watchedServices.constEnd())
            return it->owner;
    }

    // not cached
    return getNameOwnerNoCache(serviceName);
}

QString QDBusConnectionPrivate::getNameOwnerNoCache(const QString &serviceName)
{
    QDBusMessage msg = QDBusMessage::createMethodCall(QDBusUtil::dbusService(),
            QDBusUtil::dbusPath(), QDBusUtil::dbusInterface(),
            QStringLiteral("GetNameOwner"));
    QDBusMessagePrivate::setParametersValidated(msg, true);
    msg << serviceName;

    QDBusPendingCallPrivate *pcall = sendWithReplyAsync(msg, nullptr, nullptr, nullptr);
    if (thread() == QThread::currentThread()) {
        // this function may be called in our own thread and
        // QDBusPendingCallPrivate::waitForFinished() would deadlock there
        q_dbus_pending_call_block(pcall->pending);
    }
    pcall->waitForFinished();
    msg = pcall->replyMessage;

    if (!pcall->ref.deref())
        delete pcall;

    if (msg.type() == QDBusMessage::ReplyMessage)
        return msg.arguments().at(0).toString();
    return QString();
}

QDBusMetaObject *
QDBusConnectionPrivate::findMetaObject(const QString &service, const QString &path,
                                       const QString &interface, QDBusError &error)
{
    // service must be a unique connection name
    if (!interface.isEmpty()) {
        QDBusReadLocker locker(FindMetaObject1Action, this);
        QDBusMetaObject *mo = cachedMetaObjects.value(interface, 0);
        if (mo)
            return mo;
    }

    // introspect the target object
    QDBusMessage msg = QDBusMessage::createMethodCall(service, path,
                                                QDBusUtil::dbusInterfaceIntrospectable(),
                                                QStringLiteral("Introspect"));
    QDBusMessagePrivate::setParametersValidated(msg, true);

    QDBusMessage reply = sendWithReply(msg, QDBus::Block);

    // it doesn't exist yet, we have to create it
    QDBusWriteLocker locker(FindMetaObject2Action, this);
    QDBusMetaObject *mo = nullptr;
    if (!interface.isEmpty())
        mo = cachedMetaObjects.value(interface, 0);
    if (mo)
        // maybe it got created when we switched from read to write lock
        return mo;

    QString xml;
    if (reply.type() == QDBusMessage::ReplyMessage) {
        if (reply.signature() == QLatin1String("s"))
            // fetch the XML description
            xml = reply.arguments().at(0).toString();
    } else {
        error = QDBusError(reply);
        lastError = error;
        if (reply.type() != QDBusMessage::ErrorMessage || error.type() != QDBusError::UnknownMethod)
            return nullptr; // error
    }

    // release the lock and return
    QDBusMetaObject *result = QDBusMetaObject::createMetaObject(interface, xml,
                                                                cachedMetaObjects, error);
    lastError = error;
    return result;
}

void QDBusConnectionPrivate::registerService(const QString &serviceName)
{
    QDBusWriteLocker locker(RegisterServiceAction, this);
    registerServiceNoLock(serviceName);
}

void QDBusConnectionPrivate::registerServiceNoLock(const QString &serviceName)
{
    serviceNames.append(serviceName);
}

void QDBusConnectionPrivate::unregisterService(const QString &serviceName)
{
    QDBusWriteLocker locker(UnregisterServiceAction, this);
    unregisterServiceNoLock(serviceName);
}

void QDBusConnectionPrivate::unregisterServiceNoLock(const QString &serviceName)
{
    serviceNames.removeAll(serviceName);
}

bool QDBusConnectionPrivate::isServiceRegisteredByThread(const QString &serviceName)
{
    if (!serviceName.isEmpty() && serviceName == baseService)
        return true;
    if (serviceName == QDBusUtil::dbusService())
        return false;

    QDBusReadLocker locker(UnregisterServiceAction, this);
    return serviceNames.contains(serviceName);
}

void QDBusConnectionPrivate::postEventToThread(int action, QObject *object, QEvent *ev)
{
    QDBusLockerBase::reportThreadAction(action, QDBusLockerBase::BeforePost, this);
    QCoreApplication::postEvent(object, ev);
    QDBusLockerBase::reportThreadAction(action, QDBusLockerBase::AfterPost, this);
}

QT_END_NAMESPACE

#endif // QT_NO_DBUS
