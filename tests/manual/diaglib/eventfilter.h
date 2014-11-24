/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the test suite of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia. For licensing terms and
** conditions see http://qt.digia.com/licensing. For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights. These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef _EVENTFILTER_
#define _EVENTFILTER_

#include <QtCore/QObject>
#include <QtCore/QEvent>
#include <QtCore/QList>

namespace QtDiag {

// Event filter that can for example be installed on QApplication
// to log relevant events.

class EventFilter : public QObject {
public:
    enum EventCategory {
        MouseEvents        = 0x00001,
        MouseMoveEvents    = 0x00002,
        TouchEvents        = 0x00004,
        TabletEvents       = 0x00008,
        DragAndDropEvents  = 0x00010,
        KeyEvents          = 0x00020,
        GeometryEvents     = 0x00040,
        PaintEvents        = 0x00080,
        TimerEvents        = 0x00100,
        ObjectEvents       = 0x00200
    };
    Q_DECLARE_FLAGS(EventCategories, EventCategory)

    explicit EventFilter(EventCategories eventCategories, QObject *p = 0);
    explicit EventFilter(QObject *p = 0);

    bool eventFilter(QObject *, QEvent *);

private:
    void init(EventCategories eventCategories);

    QList<QEvent::Type> m_eventTypes;
};

Q_DECLARE_OPERATORS_FOR_FLAGS(EventFilter::EventCategories)

} // namespace QtDiag

#endif
