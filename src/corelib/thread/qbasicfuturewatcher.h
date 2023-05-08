// Copyright (C) 2023 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QBASICFUTUREWATCHER_H
#define QBASICFUTUREWATCHER_H

#include <QtCore/qobject.h>

QT_REQUIRE_CONFIG(future);

QT_BEGIN_NAMESPACE

class QFutureInterfaceBase;

namespace QtPrivate {

class QBasicFutureWatcherPrivate;

class Q_CORE_EXPORT QBasicFutureWatcher : public QObject
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QBasicFutureWatcher)
public:
    explicit QBasicFutureWatcher(QObject *parent = nullptr);
    ~QBasicFutureWatcher() override;

    void setFuture(QFutureInterfaceBase &fi);

    bool event(QEvent *event) override;

Q_SIGNALS:
    void finished();
};

}

QT_END_NAMESPACE

#endif // QBASICFUTUREWATCHER_H
