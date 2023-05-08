// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qbasicfuturewatcher.h"
#include "qcoreapplication.h"
#include "qfutureinterface.h"
#include "qfutureinterface_p.h"

#include <QtCore/private/qobject_p.h>

QT_BEGIN_NAMESPACE

namespace QtPrivate {

class QBasicFutureWatcherPrivate : public QObjectPrivate, QFutureCallOutInterface
{
public:
    Q_DECLARE_PUBLIC(QBasicFutureWatcher)

    QFutureInterfaceBase future;

    void postCallOutEvent(const QFutureCallOutEvent &event) override;
    void callOutInterfaceDisconnected() override;
};

void QBasicFutureWatcherPrivate::postCallOutEvent(const QFutureCallOutEvent &event)
{
    Q_Q(QBasicFutureWatcher);
    if (q->thread() == QThread::currentThread()) {
        // If we are in the same thread, don't queue up anything.
        std::unique_ptr<QFutureCallOutEvent> clonedEvent(event.clone());
        QCoreApplication::sendEvent(q, clonedEvent.get());
    } else {
        QCoreApplication::postEvent(q, event.clone());
    }
}

void QBasicFutureWatcherPrivate::callOutInterfaceDisconnected()
{
    Q_Q(QBasicFutureWatcher);
    QCoreApplication::removePostedEvents(q, QEvent::FutureCallOut);
}

/*
 * QBasicFutureWatcher is a more lightweight version of QFutureWatcher for internal use
 */
QBasicFutureWatcher::QBasicFutureWatcher(QObject *parent)
    : QObject(*new QBasicFutureWatcherPrivate, parent)
{
}

QBasicFutureWatcher::~QBasicFutureWatcher()
{
    Q_D(QBasicFutureWatcher);
    d->future.d->disconnectOutputInterface(d);
}

void QBasicFutureWatcher::setFuture(QFutureInterfaceBase &fi)
{
    Q_D(QBasicFutureWatcher);
    d->future = fi;
    d->future.d->connectOutputInterface(d);
}

bool QtPrivate::QBasicFutureWatcher::event(QEvent *event)
{
    if (event->type() == QEvent::FutureCallOut) {
        QFutureCallOutEvent *callOutEvent = static_cast<QFutureCallOutEvent *>(event);
        if (callOutEvent->callOutType == QFutureCallOutEvent::Finished)
            emit finished();
        return true;
    }
    return QObject::event(event);
}

} // namespace QtPrivate

QT_END_NAMESPACE

#include "moc_qbasicfuturewatcher.cpp"
