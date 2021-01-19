/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

#ifndef QWINEVENTNOTIFIER_P_H
#define QWINEVENTNOTIFIER_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qwineventnotifier.h"

#include <private/qobject_p.h>
#include <QtCore/qatomic.h>
#include <QtCore/qt_windows.h>

QT_BEGIN_NAMESPACE

class QWinEventNotifierPrivate : public QObjectPrivate
{
    Q_DECLARE_PUBLIC(QWinEventNotifier)
public:
    QWinEventNotifierPrivate()
    : handleToEvent(0), enabled(false) {}
    QWinEventNotifierPrivate(HANDLE h, bool e)
    : handleToEvent(h), enabled(e) {}

    static QWinEventNotifierPrivate *get(QWinEventNotifier *q) { return q->d_func(); }
    bool registerWaitObject();
    void unregisterWaitObject();

    HANDLE handleToEvent;
    HANDLE waitHandle = NULL;
    QAtomicInt signaledCount;
    bool enabled;
};

QT_END_NAMESPACE

#endif // QWINEVENTNOTIFIER_P_H
