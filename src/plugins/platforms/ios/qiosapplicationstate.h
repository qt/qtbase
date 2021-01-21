/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:COMM$
**
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** $QT_END_LICENSE$
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
**
****************************************************************************/

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
