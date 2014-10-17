/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QCOREPEPPEREVENTDISPATCHER_H
#define QCOREPEPPEREVENTDISPATCHER_H

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

#include "QtCore/qabstracteventdispatcher.h"
#include "qabstracteventdispatcher_p.h"
#include <QtCore/qloggingcategory.h>

#include <ppapi/cpp/message_loop.h>
#include <ppapi/utility/completion_callback_factory.h>

QT_BEGIN_NAMESPACE
    
Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_COREEVENTDISPATHCER);

class QCorePepperEventDispatcherPrivate;
class Q_CORE_EXPORT QCorePepperEventDispatcher : public QAbstractEventDispatcher
{
    Q_OBJECT
    Q_DECLARE_PRIVATE(QCorePepperEventDispatcher)

public:
    explicit QCorePepperEventDispatcher(QObject *parent = 0);
    ~QCorePepperEventDispatcher();

    bool processEvents(QEventLoop::ProcessEventsFlags flags) Q_DECL_OVERRIDE;
    bool hasPendingEvents() Q_DECL_OVERRIDE;

    void registerSocketNotifier(QSocketNotifier *notifier) Q_DECL_OVERRIDE;
    void unregisterSocketNotifier(QSocketNotifier *notifier) Q_DECL_OVERRIDE;

    void registerTimer(int timerId, int interval, Qt::TimerType timerType, QObject *object) Q_DECL_OVERRIDE;
    bool unregisterTimer(int timerId) Q_DECL_OVERRIDE;
    bool unregisterTimers(QObject *object) Q_DECL_OVERRIDE;
    QList<TimerInfo> registeredTimers(QObject *object) const Q_DECL_OVERRIDE;
    int remainingTime(int timerId) Q_DECL_OVERRIDE;

    void wakeUp() Q_DECL_OVERRIDE;
    void interrupt() Q_DECL_OVERRIDE;
    void flush() Q_DECL_OVERRIDE;

    void startingUp() Q_DECL_OVERRIDE;
    void closingDown() Q_DECL_OVERRIDE;

    bool event(QEvent *e) Q_DECL_OVERRIDE;

protected:
    QCorePepperEventDispatcher(QCorePepperEventDispatcherPrivate &dd, QObject *parent = 0);
    void sendPostedEvents();

private:
};

class Q_CORE_EXPORT QCorePepperEventDispatcherPrivate : public QAbstractEventDispatcherPrivate
{
    Q_DECLARE_PUBLIC(QCorePepperEventDispatcher)
public:
    QCorePepperEventDispatcherPrivate();
    ~QCorePepperEventDispatcherPrivate();

    void scheduleProcessEvents();
    void processEvents(int32_t status);

private:
    pp::MessageLoop messageLoop;
    pp::CompletionCallbackFactory<QCorePepperEventDispatcherPrivate> callbackFactory;
};

QT_END_NAMESPACE

#endif // QCOREPEPPEREVENTDISPATCHER_H
