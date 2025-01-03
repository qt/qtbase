// Copyright (C) 2019 The Qt Company Ltd.
// Copyright (C) 2013 Olivier Goffart <ogoffart@woboq.com>
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOBJECT_P_P_H
#define QOBJECT_P_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists for the convenience
// of qobject.cpp.  This header file may change from version to version
// without notice, or even be removed.
//
// We mean it.
//

// Even though this file is only used by qobject.cpp, the only reason this
// code lives here is that some special apps/libraries for e.g., QtJambi,
// Gammaray need access to the structs in this file.

#include <QtCore/qobject.h>
#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

// ConnectionList is a singly-linked list
struct QObjectPrivate::ConnectionList
{
    QAtomicPointer<Connection> first;
    QAtomicPointer<Connection> last;
};
static_assert(std::is_trivially_destructible_v<QObjectPrivate::ConnectionList>);
Q_DECLARE_TYPEINFO(QObjectPrivate::ConnectionList, Q_RELOCATABLE_TYPE);

struct QObjectPrivate::ConnectionOrSignalVector
{
    union {
        // linked list of orphaned connections that need cleaning up
        ConnectionOrSignalVector *nextInOrphanList;
        // linked list of connections connected to slots in this object
        Connection *next;
    };

    static SignalVector *asSignalVector(ConnectionOrSignalVector *c)
    {
        if (reinterpret_cast<quintptr>(c) & 1)
            return reinterpret_cast<SignalVector *>(reinterpret_cast<quintptr>(c) & ~quintptr(1u));
        return nullptr;
    }
    static Connection *fromSignalVector(SignalVector *v)
    {
        return reinterpret_cast<Connection *>(reinterpret_cast<quintptr>(v) | quintptr(1u));
    }
};
static_assert(std::is_trivial_v<QObjectPrivate::ConnectionOrSignalVector>);

struct QObjectPrivate::Connection : public ConnectionOrSignalVector
{
    // linked list of connections connected to slots in this object, next is in base class
    Connection **prev;
    // linked list of connections connected to signals in this object
    QAtomicPointer<Connection> nextConnectionList;
    Connection *prevConnectionList;

    QObject *sender;
    QAtomicPointer<QObject> receiver;
    QAtomicPointer<QThreadData> receiverThreadData;
    union {
        StaticMetaCallFunction callFunction;
        QtPrivate::QSlotObjectBase *slotObj;
    };
    QAtomicPointer<const int> argumentTypes;
    QAtomicInt ref_{
        2
    }; // ref_ is 2 for the use in the internal lists, and for the use in QMetaObject::Connection
    uint id = 0;
    ushort method_offset;
    ushort method_relative;
    signed int signal_index : 27; // In signal range (see QObjectPrivate::signalIndex())
    ushort connectionType : 2; // 0 == auto, 1 == direct, 2 == queued, 3 == blocking
    ushort isSlotObject : 1;
    ushort ownArgumentTypes : 1;
    ushort isSingleShot : 1;
    Connection() : ownArgumentTypes(true) { }
    ~Connection();
    int method() const
    {
        Q_ASSERT(!isSlotObject);
        return method_offset + method_relative;
    }
    void ref() { ref_.ref(); }
    void freeSlotObject()
    {
        if (isSlotObject) {
            slotObj->destroyIfLastRef();
            isSlotObject = false;
        }
    }
    void deref()
    {
        if (!ref_.deref()) {
            Q_ASSERT(!receiver.loadRelaxed());
            Q_ASSERT(!isSlotObject);
            delete this;
        }
    }
};
Q_DECLARE_TYPEINFO(QObjectPrivate::Connection, Q_RELOCATABLE_TYPE);

struct QObjectPrivate::SignalVector : public ConnectionOrSignalVector
{
    quintptr allocated;
    // ConnectionList signals[]
    ConnectionList &at(int i) { return reinterpret_cast<ConnectionList *>(this + 1)[i + 1]; }
    const ConnectionList &at(int i) const
    {
        return reinterpret_cast<const ConnectionList *>(this + 1)[i + 1];
    }
    int count() const { return static_cast<int>(allocated); }
};
static_assert(
        std::is_trivial_v<QObjectPrivate::SignalVector>); // it doesn't need to be, but it helps

struct QObjectPrivate::ConnectionData
{
    // the id below is used to avoid activating new connections. When the object gets
    // deleted it's set to 0, so that signal emission stops
    QAtomicInteger<uint> currentConnectionId;
    QAtomicInt ref;
    QAtomicPointer<SignalVector> signalVector;
    Connection *senders = nullptr;
    Sender *currentSender = nullptr; // object currently activating the object
    QAtomicPointer<Connection> orphaned;

    ~ConnectionData()
    {
        Q_ASSERT(ref.loadRelaxed() == 0);
        auto *c = orphaned.fetchAndStoreRelaxed(nullptr);
        if (c)
            deleteOrphaned(c);
        SignalVector *v = signalVector.loadRelaxed();
        if (v) {
            v->~SignalVector();
            free(v);
        }
    }

    // must be called on the senders connection data
    // assumes the senders and receivers lock are held
    void removeConnection(Connection *c);
    enum LockPolicy {
        NeedToLock,
        // Beware that we need to temporarily release the lock
        // and thus calling code must carefully consider whether
        // invariants still hold.
        AlreadyLockedAndTemporarilyReleasingLock
    };
    void cleanOrphanedConnections(QObject *sender, LockPolicy lockPolicy = NeedToLock)
    {
        if (orphaned.loadRelaxed() && ref.loadAcquire() == 1)
            cleanOrphanedConnectionsImpl(sender, lockPolicy);
    }
    void cleanOrphanedConnectionsImpl(QObject *sender, LockPolicy lockPolicy);

    ConnectionList &connectionsForSignal(int signal)
    {
        return signalVector.loadRelaxed()->at(signal);
    }

    void resizeSignalVector(uint size)
    {
        SignalVector *vector = this->signalVector.loadRelaxed();
        if (vector && vector->allocated > size)
            return;
        size = (size + 7) & ~7;
        void *ptr = malloc(sizeof(SignalVector) + (size + 1) * sizeof(ConnectionList));
        auto newVector = new (ptr) SignalVector;

        int start = -1;
        if (vector) {
            // not (yet) existing trait:
            // static_assert(std::is_relocatable_v<SignalVector>);
            // static_assert(std::is_relocatable_v<ConnectionList>);
            memcpy(newVector, vector,
                   sizeof(SignalVector) + (vector->allocated + 1) * sizeof(ConnectionList));
            start = vector->count();
        }
        for (int i = start; i < int(size); ++i)
            new (&newVector->at(i)) ConnectionList();
        newVector->next = nullptr;
        newVector->allocated = size;

        signalVector.storeRelaxed(newVector);
        if (vector) {
            Connection *o = nullptr;
            /* No ABA issue here: When adding a node, we only care about the list head, it doesn't
             * matter if the tail changes.
             */
            do {
                o = orphaned.loadRelaxed();
                vector->nextInOrphanList = o;
            } while (!orphaned.testAndSetRelease(
                    o, ConnectionOrSignalVector::fromSignalVector(vector)));
        }
    }
    int signalVectorCount() const
    {
        return signalVector.loadAcquire() ? signalVector.loadRelaxed()->count() : -1;
    }

    static void deleteOrphaned(ConnectionOrSignalVector *c);
};

struct QObjectPrivate::Sender
{
    Sender(QObject *receiver, QObject *sender, int signal)
        : receiver(receiver), sender(sender), signal(signal)
    {
        if (receiver) {
            ConnectionData *cd = receiver->d_func()->connections.loadRelaxed();
            previous = cd->currentSender;
            cd->currentSender = this;
        }
    }
    ~Sender()
    {
        if (receiver)
            receiver->d_func()->connections.loadRelaxed()->currentSender = previous;
    }
    void receiverDeleted()
    {
        Sender *s = this;
        while (s) {
            s->receiver = nullptr;
            s = s->previous;
        }
    }
    Sender *previous;
    QObject *receiver;
    QObject *sender;
    int signal;
};
Q_DECLARE_TYPEINFO(QObjectPrivate::Sender, Q_RELOCATABLE_TYPE);

QT_END_NAMESPACE

#endif
