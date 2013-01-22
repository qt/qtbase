/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
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
