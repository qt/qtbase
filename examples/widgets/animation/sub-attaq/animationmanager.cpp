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

//Own
#include "animationmanager.h"

//Qt
#include <QtCore/QAbstractAnimation>
#include <QtCore/QDebug>

// the universe's only animation manager
AnimationManager *AnimationManager::instance = 0;

AnimationManager::AnimationManager()
{
}

AnimationManager *AnimationManager::self()
{
    if (!instance)
        instance = new AnimationManager;
    return instance;
}

void AnimationManager::registerAnimation(QAbstractAnimation *anim)
{
    QObject::connect(anim, SIGNAL(destroyed(QObject*)), this, SLOT(unregisterAnimation_helper(QObject*)));
    animations.append(anim);
}

void AnimationManager::unregisterAnimation_helper(QObject *obj)
{
    unregisterAnimation(static_cast<QAbstractAnimation*>(obj));
}

void AnimationManager::unregisterAnimation(QAbstractAnimation *anim)
{
    QObject::disconnect(anim, SIGNAL(destroyed(QObject*)), this, SLOT(unregisterAnimation_helper(QObject*)));
    animations.removeAll(anim);
}

void AnimationManager::unregisterAllAnimations()
{
    animations.clear();
}

void AnimationManager::pauseAll()
{
    foreach (QAbstractAnimation* animation, animations) {
        if (animation->state() == QAbstractAnimation::Running)
            animation->pause();
    }
}
void AnimationManager::resumeAll()
{
    foreach (QAbstractAnimation* animation, animations) {
        if (animation->state() == QAbstractAnimation::Paused)
            animation->resume();
    }
}
