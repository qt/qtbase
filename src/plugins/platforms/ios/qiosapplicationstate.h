// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSAPPLICATIONSTATE_H
#define QIOSAPPLICATIONSTATE_H

#include <QtCore/qobject.h>

#include <UIKit/UIApplication.h>

QT_BEGIN_NAMESPACE

class QIOSApplicationState : public QObject
{
    Q_OBJECT
public:
    QIOSApplicationState();

    static void handleApplicationStateChanged(UIApplicationState state, const QString &reason);
    static Qt::ApplicationState toQtApplicationState(UIApplicationState state);

Q_SIGNALS:
    void applicationStateWillChange(Qt::ApplicationState oldState, Qt::ApplicationState newState);
    void applicationStateDidChange(Qt::ApplicationState oldState, Qt::ApplicationState newState);
};

QT_END_NAMESPACE

#endif
