// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwidgetanimator_p.h"

#if QT_CONFIG(animation)
#include <QtCore/qpropertyanimation.h>
#endif
#include <QtWidgets/qwidget.h>
#include <QtWidgets/qstyle.h>
#if QT_CONFIG(mainwindow)
#include <private/qmainwindowlayout_p.h>
#endif

QT_BEGIN_NAMESPACE

QWidgetAnimator::QWidgetAnimator(QMainWindowLayout *layout)
#if QT_CONFIG(mainwindow)
: m_mainWindowLayout(layout)
#endif
{
    Q_UNUSED(layout);
}

void QWidgetAnimator::abort(QWidget *w)
{
#if QT_CONFIG(animation)
    const auto it = m_animation_map.constFind(w);
    if (it == m_animation_map.cend())
        return;
    QPropertyAnimation *anim = *it;
    m_animation_map.erase(it);
    if (anim) {
        anim->stop();
    }
#if QT_CONFIG(mainwindow)
    m_mainWindowLayout->animationFinished(w);
#endif
#else
    Q_UNUSED(w); //there is no animation to abort
#endif // animation
}

#if QT_CONFIG(animation)
void QWidgetAnimator::animationFinished()
{
    QPropertyAnimation *anim = qobject_cast<QPropertyAnimation*>(sender());
    abort(static_cast<QWidget*>(anim->targetObject()));
}
#endif // animation

void QWidgetAnimator::animate(QWidget *widget, const QRect &_final_geometry, bool animate)
{
    QRect r = widget->geometry();
    if (r.right() < 0 || r.bottom() < 0)
        r = QRect();

    animate = animate && !r.isNull() && !_final_geometry.isNull();

    // might make the widget go away by sending it to negative space
    const QRect final_geometry = _final_geometry.isValid() || widget->isWindow() ? _final_geometry :
        QRect(QPoint(-500 - widget->width(), -500 - widget->height()), widget->size());

#if QT_CONFIG(animation)
    //If the QStyle has animations, animate
    if (const int animationDuration = widget->style()->styleHint(QStyle::SH_Widget_Animation_Duration, nullptr, widget)) {
        AnimationMap::const_iterator it = m_animation_map.constFind(widget);
        if (it != m_animation_map.constEnd() && (*it)->endValue().toRect() == final_geometry)
            return;

        QPropertyAnimation *anim = new QPropertyAnimation(widget, "geometry", widget);
        anim->setDuration(animate ? animationDuration : 0);
        anim->setEasingCurve(QEasingCurve::InOutQuad);
        anim->setEndValue(final_geometry);
        m_animation_map[widget] = anim;
        connect(anim, &QPropertyAnimation::finished,
                this, &QWidgetAnimator::animationFinished);
        anim->start(QPropertyAnimation::DeleteWhenStopped);
    } else
#endif // animation
    {
    //we do it in one shot
    widget->setGeometry(final_geometry);
#if QT_CONFIG(mainwindow)
    m_mainWindowLayout->animationFinished(widget);
#endif // QT_CONFIG(mainwindow)
    }
}

bool QWidgetAnimator::animating() const
{
    return !m_animation_map.isEmpty();
}

QT_END_NAMESPACE

#include "moc_qwidgetanimator_p.cpp"
