/***************************************************************************
**
** Copyright (C) 2011 - 2012 Research In Motion
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQNXSCREENEVENTTHREAD_H
#define QQNXSCREENEVENTTHREAD_H

#include <QtCore/QThread>
#include <QtCore/QMutex>

#include <screen/screen.h>

QT_BEGIN_NAMESPACE

class QQnxScreenEventHandler;

typedef QVarLengthArray<screen_event_t, 64> QQnxScreenEventArray;

class QQnxScreenEventThread : public QThread
{
    Q_OBJECT

public:
    QQnxScreenEventThread(screen_context_t context, QQnxScreenEventHandler *screenEventHandler);
    ~QQnxScreenEventThread();

    static void injectKeyboardEvent(int flags, int sym, int mod, int scan, int cap);

    QQnxScreenEventArray *lock();
    void unlock();

protected:
    void run();

Q_SIGNALS:
    void eventPending();

private:
    void shutdown();

    screen_context_t m_screenContext;
    QMutex m_mutex;
    QQnxScreenEventArray m_events;
    QQnxScreenEventHandler *m_screenEventHandler;
    bool m_quit;
};

QT_END_NAMESPACE

#endif // QQNXSCREENEVENTTHREAD_H
