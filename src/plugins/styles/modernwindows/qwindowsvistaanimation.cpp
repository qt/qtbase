// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial

#include "qwindowsvistaanimation_p.h"
#include "qwindowsvistastyle_p_p.h"

bool QWindowsVistaAnimation::isUpdateNeeded() const
{
    return QWindowsVistaStylePrivate::useVista();
}

void QWindowsVistaAnimation::paint(QPainter *painter, const QStyleOption *option)
{
    painter->drawImage(option->rect, currentImage());
}
