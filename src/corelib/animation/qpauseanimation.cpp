/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtCore module of the Qt Toolkit.
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

/*!
    \class QPauseAnimation
    \inmodule QtCore
    \brief The QPauseAnimation class provides a pause for QSequentialAnimationGroup.
    \since 4.6
    \ingroup animation

    If you wish to introduce a delay between animations in a
    QSequentialAnimationGroup, you can insert a QPauseAnimation. This
    class does not animate anything, but does not
    \l{QAbstractAnimation::finished()}{finish} before a specified
    number of milliseconds have elapsed from when it was started. You
    specify the duration of the pause in the constructor. It can also
    be set directly with setDuration().

    It is not necessary to construct a QPauseAnimation yourself.
    QSequentialAnimationGroup provides the convenience functions
    \l{QSequentialAnimationGroup::}{addPause()} and
    \l{QSequentialAnimationGroup::}{insertPause()}. These functions
    simply take the number of milliseconds the pause should last.

    \sa QSequentialAnimationGroup
*/

#include "qpauseanimation.h"
#include "qabstractanimation_p.h"

QT_BEGIN_NAMESPACE

class QPauseAnimationPrivate : public QAbstractAnimationPrivate
{
public:
    QPauseAnimationPrivate() : QAbstractAnimationPrivate(), duration(250)
    {
        isPause = true;
    }

    int duration;
};

/*!
    Constructs a QPauseAnimation.
    \a parent is passed to QObject's constructor.
    The default duration is 0.
*/

QPauseAnimation::QPauseAnimation(QObject *parent) : QAbstractAnimation(*new QPauseAnimationPrivate, parent)
{
}

/*!
    Constructs a QPauseAnimation.
    \a msecs is the duration of the pause.
    \a parent is passed to QObject's constructor.
*/

QPauseAnimation::QPauseAnimation(int msecs, QObject *parent) : QAbstractAnimation(*new QPauseAnimationPrivate, parent)
{
    setDuration(msecs);
}

/*!
    Destroys the pause animation.
*/
QPauseAnimation::~QPauseAnimation()
{
}

/*!
    \property QPauseAnimation::duration
    \brief the duration of the pause.

    The duration of the pause. The duration should not be negative.
    The default duration is 250 milliseconds.
*/
int QPauseAnimation::duration() const
{
    Q_D(const QPauseAnimation);
    return d->duration;
}

void QPauseAnimation::setDuration(int msecs)
{
    if (msecs < 0) {
        qWarning("QPauseAnimation::setDuration: cannot set a negative duration");
        return;
    }
    Q_D(QPauseAnimation);
    d->duration = msecs;
}

/*!
    \reimp
 */
bool QPauseAnimation::event(QEvent *e)
{
    return QAbstractAnimation::event(e);
}

/*!
    \reimp
 */
void QPauseAnimation::updateCurrentTime(int)
{
}


QT_END_NAMESPACE

#include "moc_qpauseanimation.cpp"
