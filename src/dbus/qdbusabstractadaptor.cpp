// Copyright (C) 2016 The Qt Company Ltd.
// Copyright (C) 2016 Intel Corporation.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qdbusabstractadaptor.h"
#include "qdbusabstractadaptor_p.h"

#include <QtCore/qcoreapplication.h>
#include <QtCore/qmetaobject.h>
#include <QtCore/qset.h>
#include <QtCore/qtimer.h>
#include <QtCore/qthread.h>

#include "qdbusconnection.h"

#include "qdbusconnection_p.h"  // for qDBusParametersForMethod
#include "qdbusmetatype_p.h"

#include <algorithm>

#ifndef QT_NO_DBUS

QT_BEGIN_NAMESPACE

static int cachedRelaySlotMethodIndex = 0;

int QDBusAdaptorConnector::relaySlotMethodIndex()
{
    if (cachedRelaySlotMethodIndex == 0) {
        cachedRelaySlotMethodIndex = staticMetaObject.indexOfMethod("relaySlot()");
        Q_ASSERT(cachedRelaySlotMethodIndex != 0); // 0 should be deleteLater() or destroyed()
    }
    return cachedRelaySlotMethodIndex;
}

QDBusAdaptorConnector *qDBusFindAdaptorConnector(QObject *obj)
{
    if (!obj)
        return nullptr;

    for (QObject *child : std::as_const(obj->children())) {
        QDBusAdaptorConnector *connector = qobject_cast<QDBusAdaptorConnector *>(child);
        if (connector) {
            connector->polish();
            return connector;
        }
    }
    return nullptr;
}

QDBusAdaptorConnector *qDBusFindAdaptorConnector(QDBusAbstractAdaptor *adaptor)
{
    return qDBusFindAdaptorConnector(adaptor->parent());
}

QDBusAdaptorConnector *qDBusCreateAdaptorConnector(QObject *obj)
{
    QDBusAdaptorConnector *connector = qDBusFindAdaptorConnector(obj);
    if (connector)
        return connector;
    return new QDBusAdaptorConnector(obj);
}

QString QDBusAbstractAdaptorPrivate::retrieveIntrospectionXml(QDBusAbstractAdaptor *adaptor)
{
    return adaptor->d_func()->xml;
}

void QDBusAbstractAdaptorPrivate::saveIntrospectionXml(QDBusAbstractAdaptor *adaptor,
                                                       const QString &xml)
{
    adaptor->d_func()->xml = xml;
}

/*!
    \class QDBusAbstractAdaptor
    \inmodule QtDBus
    \since 4.2

    \brief The QDBusAbstractAdaptor class is the base class of D-Bus adaptor classes.

    The QDBusAbstractAdaptor class is the starting point for all objects intending to provide
    interfaces to the external world using D-Bus. This is accomplished by attaching a one or more
    classes derived from QDBusAbstractAdaptor to a normal QObject and then registering that QObject
    with QDBusConnection::registerObject. QDBusAbstractAdaptor objects are intended to be
    light-weight wrappers, mostly just relaying calls into the real object (its parent) and the
    signals from it.

    Each QDBusAbstractAdaptor-derived class should define the D-Bus interface it is implementing
    using the Q_CLASSINFO macro in the class definition. Note that only one interface can be
    exposed in this way.

    QDBusAbstractAdaptor uses the standard QObject mechanism of signals, slots and properties to
    determine what signals, methods and properties to export to the bus. Any signal emitted by
    QDBusAbstractAdaptor-derived classes will be automatically be relayed through any D-Bus
    connections the object is registered on.

    Classes derived from QDBusAbstractAdaptor must be created on the heap using the \a new operator
    and must not be deleted by the user (they will be deleted automatically when the object they are
    connected to is also deleted).

    \sa {usingadaptors.html}{Using adaptors}, QDBusConnection
*/

/*!
    Constructs a QDBusAbstractAdaptor with \a obj as the parent object.
*/
QDBusAbstractAdaptor::QDBusAbstractAdaptor(QObject* obj)
    : QObject(*new QDBusAbstractAdaptorPrivate, obj)
{
    QDBusAdaptorConnector *connector = qDBusCreateAdaptorConnector(obj);

    connector->waitingForPolish = true;
    QMetaObject::invokeMethod(connector, &QDBusAdaptorConnector::polish, Qt::QueuedConnection);
}

/*!
    Destroys the adaptor.

    \warning Adaptors are destroyed automatically when the real object they refer to is
             destroyed. Do not delete the adaptors yourself.
*/
QDBusAbstractAdaptor::~QDBusAbstractAdaptor()
{
}

/*!
    Toggles automatic signal relaying from the real object (see object()).

    Automatic signal relaying consists of signal-to-signal connection of the signals on the parent
    that have the exact same method signature in both classes.

    If \a enable is set to true, connect the signals; if set to false, disconnect all signals.
*/
void QDBusAbstractAdaptor::setAutoRelaySignals(bool enable)
{
    const QMetaObject *us = metaObject();
    const QMetaObject *them = parent()->metaObject();
    bool connected = false;
    for (int idx = staticMetaObject.methodCount(); idx < us->methodCount(); ++idx) {
        QMetaMethod mm = us->method(idx);

        if (mm.methodType() != QMetaMethod::Signal)
            continue;

        // try to connect/disconnect to a signal on the parent that has the same method signature
        QByteArray sig = QMetaObject::normalizedSignature(mm.methodSignature().constData());
        if (them->indexOfSignal(sig) == -1)
            continue;
        sig.prepend(QSIGNAL_CODE + '0');
        parent()->disconnect(sig, this, sig);
        if (enable)
            connected = connect(parent(), sig, sig) || connected;
    }
    d_func()->autoRelaySignals = connected;
}

/*!
    Returns \c true if automatic signal relaying from the real object (see object()) is enabled,
    otherwiser returns \c false.

    \sa setAutoRelaySignals()
*/
bool QDBusAbstractAdaptor::autoRelaySignals() const
{
    return d_func()->autoRelaySignals;
}

QDBusAdaptorConnector::QDBusAdaptorConnector(QObject *obj)
    : QObject(obj), waitingForPolish(false)
{
}

QDBusAdaptorConnector::~QDBusAdaptorConnector()
{
}

void QDBusAdaptorConnector::addAdaptor(QDBusAbstractAdaptor *adaptor)
{
    // find the interface name
    const QMetaObject *mo = adaptor->metaObject();
    int ciid = mo->indexOfClassInfo(QCLASSINFO_DBUS_INTERFACE);
    if (ciid != -1) {
        QMetaClassInfo mci = mo->classInfo(ciid);
        if (*mci.value()) {
            // find out if this interface exists first
            const char *interface = mci.value();
            AdaptorMap::Iterator it = std::lower_bound(adaptors.begin(), adaptors.end(),
                                                       QByteArray(interface));
            if (it != adaptors.end() && qstrcmp(interface, it->interface) == 0) {
                // exists. Replace it (though it's probably the same)
                if (it->adaptor != adaptor) {
                    // reconnect the signals
                    disconnectAllSignals(it->adaptor);
                    connectAllSignals(adaptor);
                }
                it->adaptor = adaptor;
            } else {
                // create a new one
                AdaptorData entry;
                entry.interface = interface;
                entry.adaptor = adaptor;
                adaptors << entry;

                // connect the adaptor's signals to our relaySlot slot
                connectAllSignals(adaptor);
            }
        }
    }
}

void QDBusAdaptorConnector::disconnectAllSignals(QObject *obj)
{
    QMetaObject::disconnect(obj, -1, this, relaySlotMethodIndex());
}

void QDBusAdaptorConnector::connectAllSignals(QObject *obj)
{
    QMetaObject::connect(obj, -1, this, relaySlotMethodIndex(), Qt::DirectConnection);
}

void QDBusAdaptorConnector::polish()
{
    if (!waitingForPolish)
        return;                 // avoid working multiple times if multiple adaptors were added

    waitingForPolish = false;
    for (QObject *child : std::as_const(parent()->children())) {
        QDBusAbstractAdaptor *adaptor = qobject_cast<QDBusAbstractAdaptor *>(child);
        if (adaptor)
            addAdaptor(adaptor);
    }

    // sort the adaptor list
    std::sort(adaptors.begin(), adaptors.end());
}

void QDBusAdaptorConnector::relaySlot(QMethodRawArguments argv)
{
    QObject *sndr = sender();
    if (Q_LIKELY(sndr)) {
        relay(sndr, senderSignalIndex(), argv.arguments);
    } else {
        qWarning("QtDBus: cannot relay signals from parent %s(%p \"%s\") unless they are emitted in the object's thread %s(%p \"%s\"). "
                 "Current thread is %s(%p \"%s\").",
                 parent()->metaObject()->className(), parent(), qPrintable(parent()->objectName()),
                 parent()->thread()->metaObject()->className(), parent()->thread(), qPrintable(parent()->thread()->objectName()),
                 QThread::currentThread()->metaObject()->className(), QThread::currentThread(), qPrintable(QThread::currentThread()->objectName()));
    }
}

void QDBusAdaptorConnector::relay(QObject *senderObj, int lastSignalIdx, void **argv)
{
    if (lastSignalIdx < QObject::staticMetaObject.methodCount())
        // QObject signal (destroyed(QObject *)) -- ignore
        return;

    QMetaMethod mm = senderObj->metaObject()->method(lastSignalIdx);
    const QMetaObject *senderMetaObject = mm.enclosingMetaObject();

    QObject *realObject = senderObj;
    if (qobject_cast<QDBusAbstractAdaptor *>(senderObj))
        // it's an adaptor, so the real object is in fact its parent
        realObject = realObject->parent();

    // break down the parameter list
    QList<QMetaType> types;
    QString errorMsg;
    int inputCount = qDBusParametersForMethod(mm, types, errorMsg);
    if (inputCount == -1) {
        // invalid signal signature
        qWarning("QDBusAbstractAdaptor: Cannot relay signal %s::%s: %s",
                 senderMetaObject->className(), mm.methodSignature().constData(),
                 qPrintable(errorMsg));
        return;
    }
    if (inputCount + 1 != types.size() ||
        types.at(inputCount) == QDBusMetaTypeId::message()) {
        // invalid signal signature
        qWarning("QDBusAbstractAdaptor: Cannot relay signal %s::%s",
                 senderMetaObject->className(), mm.methodSignature().constData());
        return;
    }

    QVariantList args;
    const int numTypes = types.size();
    args.reserve(numTypes - 1);
    for (int i = 1; i < numTypes; ++i)
        args << QVariant(QMetaType(types.at(i)), argv[i]);

    // now emit the signal with all the information
    emit relaySignal(realObject, senderMetaObject, lastSignalIdx, args);
}

QT_END_NAMESPACE

#include "moc_qdbusabstractadaptor_p.cpp"
#include "moc_qdbusabstractadaptor.cpp"

#endif // QT_NO_DBUS
