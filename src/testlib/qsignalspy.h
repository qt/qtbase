// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QSIGNALSPY_H
#define QSIGNALSPY_H

#include <QtCore/qbytearray.h>
#include <QtCore/qlist.h>
#include <QtCore/qobject.h>
#include <QtCore/qmetaobject.h>
#include <QtTest/qtesteventloop.h>
#include <QtCore/qvariant.h>
#include <QtCore/qmutex.h>

QT_BEGIN_NAMESPACE


class QVariant;

class QSignalSpy: public QObject, public QList<QList<QVariant> >
{
    struct ObjectSignal {
        const QObject *obj;
        QMetaMethod sig;
    };

public:
    explicit QSignalSpy(const QObject *obj, const char *aSignal)
        : QSignalSpy(verify(obj, aSignal)) {}
#ifdef Q_QDOC
    template <typename PointerToMemberFunction>
    QSignalSpy(const QObject *object, PointerToMemberFunction signal);
#else
    template <typename Func>
    QSignalSpy(const typename QtPrivate::FunctionPointer<Func>::Object *obj, Func signal0)
        : QSignalSpy(verify(obj, QMetaMethod::fromSignal(signal0))) {}
#endif // Q_QDOC
    QSignalSpy(const QObject *obj, QMetaMethod signal)
        : QSignalSpy(verify(obj, signal)) {}

    inline bool isValid() const { return !sig.isEmpty(); }
    inline QByteArray signal() const { return sig; }

    bool wait(int timeout)
    { return wait(std::chrono::milliseconds{timeout}); }

    Q_TESTLIB_EXPORT bool wait(std::chrono::milliseconds timeout = std::chrono::seconds{5});

    int qt_metacall(QMetaObject::Call call, int methodId, void **a) override
    {
        methodId = QObject::qt_metacall(call, methodId, a);
        if (methodId < 0)
            return methodId;

        if (call == QMetaObject::InvokeMetaMethod) {
            if (methodId == 0) {
                appendArgs(a);
            }
            --methodId;
        }
        return methodId;
    }

private:
    explicit QSignalSpy(ObjectSignal os)
        : args(os.obj ? makeArgs(os.sig, os.obj) : QList<int>{})
    {
        init(os);
    }
    Q_TESTLIB_EXPORT void init(ObjectSignal os);

    Q_TESTLIB_EXPORT static ObjectSignal verify(const QObject *obj, QMetaMethod signal);
    Q_TESTLIB_EXPORT static ObjectSignal verify(const QObject *obj, const char *aSignal);

    Q_TESTLIB_EXPORT static QList<int> makeArgs(const QMetaMethod &member, const QObject *obj);
    Q_TESTLIB_EXPORT void appendArgs(void **a);

    // the full, normalized signal name
    QByteArray sig;
    // holds the QMetaType types for the argument list of the signal
    const QList<int> args;

    QTestEventLoop m_loop;
    bool m_waiting = false;
    QMutex m_mutex; // protects m_waiting and the QList base class, between appendArgs() and wait()
};

QT_END_NAMESPACE

#endif
