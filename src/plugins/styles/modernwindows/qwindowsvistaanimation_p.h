// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QWINDOWSVISTAANIMATION_P_H
#define QWINDOWSVISTAANIMATION_P_H

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

#include <private/qstyleanimation_p.h>
#include <QtWidgets/private/qwindowsstyle_p.h>

QT_BEGIN_NAMESPACE

class QWindowsVistaAnimation : public QBlendStyleAnimation
{
    Q_OBJECT
public:
    QWindowsVistaAnimation(Type type, QObject *target) : QBlendStyleAnimation(type, target) { }

    bool isUpdateNeeded() const override;
    void paint(QPainter *painter, const QStyleOption *option);
};


// Handles state transition animations
class QWindowsVistaTransition : public QWindowsVistaAnimation
{
    Q_OBJECT
public:
    QWindowsVistaTransition(QObject *target) : QWindowsVistaAnimation(Transition, target) {}
};


// Handles pulse animations (default buttons)
class QWindowsVistaPulse: public QWindowsVistaAnimation
{
    Q_OBJECT
public:
    QWindowsVistaPulse(QObject *target) : QWindowsVistaAnimation(Pulse, target) {}
};

QT_END_NAMESPACE

#endif // QWINDOWSVISTAANIMATION_P_H
