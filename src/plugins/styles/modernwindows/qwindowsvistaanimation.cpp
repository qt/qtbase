// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include "qwindowsvistaanimation_p.h"
#include "qwindowsvistastyle_p_p.h"

QT_BEGIN_NAMESPACE

bool QWindowsVistaAnimation::isUpdateNeeded() const
{
    return QWindowsVistaStylePrivate::useVista();
}

void QWindowsVistaAnimation::paint(QPainter *painter, const QStyleOption *option)
{
    painter->drawImage(option->rect, currentImage());
}

QT_END_NAMESPACE
