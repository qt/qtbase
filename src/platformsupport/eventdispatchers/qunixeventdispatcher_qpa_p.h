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

#ifndef QUNIXEVENTDISPATCHER_QPA_H
#define QUNIXEVENTDISPATCHER_QPA_H

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

#include <QtCore/qglobal.h>
#include <QtCore/private/qeventdispatcher_unix_p.h>

QT_BEGIN_NAMESPACE

class QUnixEventDispatcherQPA : public QEventDispatcherUNIX
{
    Q_OBJECT

public:
    explicit QUnixEventDispatcherQPA(QObject *parent = nullptr);
    ~QUnixEventDispatcherQPA();

    bool processEvents(QEventLoop::ProcessEventsFlags flags);
    bool hasPendingEvents();

    void flush();
};

QT_END_NAMESPACE

#endif // QUNIXEVENTDISPATCHER_QPA_H
