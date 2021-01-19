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

#ifndef QPARALLELANIMATIONGROUP_P_H
#define QPARALLELANIMATIONGROUP_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API. It exists purely as an
// implementation detail. This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include "qparallelanimationgroup.h"
#include "private/qanimationgroup_p.h"
#include <QtCore/qhash.h>

QT_REQUIRE_CONFIG(animation);

QT_BEGIN_NAMESPACE

class QParallelAnimationGroupPrivate : public QAnimationGroupPrivate
{
    Q_DECLARE_PUBLIC(QParallelAnimationGroup)
public:
    QParallelAnimationGroupPrivate()
        : lastLoop(0), lastCurrentTime(0)
    {
    }

    QHash<QAbstractAnimation*, int> uncontrolledFinishTime;
    int lastLoop;
    int lastCurrentTime;

    bool shouldAnimationStart(QAbstractAnimation *animation, bool startIfAtEnd) const;
    void applyGroupState(QAbstractAnimation *animation);
    bool isUncontrolledAnimationFinished(QAbstractAnimation *anim) const;
    void connectUncontrolledAnimations();
    void disconnectUncontrolledAnimations();

    void animationRemoved(int index, QAbstractAnimation *) override;

    // private slot
    void _q_uncontrolledAnimationFinished();
};

QT_END_NAMESPACE

#endif //QPARALLELANIMATIONGROUP_P_H
