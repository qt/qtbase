/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
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
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QHISTORYSTATE_P_H
#define QHISTORYSTATE_P_H

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

#include "private/qabstractstate_p.h"

#include <QtCore/qabstracttransition.h>
#include <QtCore/qhistorystate.h>
#include <QtCore/qlist.h>

QT_BEGIN_NAMESPACE

class QHistoryStatePrivate : public QAbstractStatePrivate
{
    Q_DECLARE_PUBLIC(QHistoryState)

public:
    QHistoryStatePrivate();

    static QHistoryStatePrivate *get(QHistoryState *q)
    { return q->d_func(); }

    QAbstractTransition *defaultTransition;
    QHistoryState::HistoryType historyType;
    QList<QAbstractState*> configuration;
};

class DefaultStateTransition: public QAbstractTransition
{
    Q_OBJECT

public:
    DefaultStateTransition(QHistoryState *source, QAbstractState *target);

protected:
    // It doesn't matter whether this transition matches any event or not. It is always associated
    // with a QHistoryState, and as soon as the state-machine detects that it enters a history
    // state, it will handle this transition as a special case. The history state itself is never
    // entered either: either the stored configuration will be used, or the target(s) of this
    // transition are used.
    virtual bool eventTest(QEvent *event) { Q_UNUSED(event); return false; }
    virtual void onTransition(QEvent *event) { Q_UNUSED(event); }
};

QT_END_NAMESPACE

#endif
