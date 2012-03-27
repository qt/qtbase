/***************************************************************************
**
** Copyright (C) 2012 Research In Motion
** Contact: http://www.qt-project.org/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QQNXBPSEVENTFILTER_H
#define QQNXBPSEVENTFILTER_H

#include <QObject>

struct bps_event_t;

QT_BEGIN_NAMESPACE

class QAbstractEventDispatcher;
class QQnxNavigatorEventHandler;
class QQnxScreen;
class QQnxScreenEventHandler;

class QQnxBpsEventFilter : public QObject
{
    Q_OBJECT
public:
    QQnxBpsEventFilter(QQnxNavigatorEventHandler *navigatorEventHandler,
                       QQnxScreenEventHandler *screenEventHandler, QObject *parent = 0);
    ~QQnxBpsEventFilter();

    void installOnEventDispatcher(QAbstractEventDispatcher *dispatcher);

    void registerForScreenEvents(QQnxScreen *screen);
    void unregisterForScreenEvents(QQnxScreen *screen);

private:
    static bool dispatcherEventFilter(void *message);
    bool bpsEventFilter(bps_event_t *event);

    bool handleNavigatorEvent(bps_event_t *event);

private:
    QQnxNavigatorEventHandler *m_navigatorEventHandler;
    QQnxScreenEventHandler *m_screenEventHandler;
};

QT_END_NAMESPACE

#endif // QQNXBPSEVENTFILTER_H
