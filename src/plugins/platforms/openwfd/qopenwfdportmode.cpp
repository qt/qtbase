/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

#include "qopenwfdportmode.h"

#include "qopenwfdport.h"

QOpenWFDPortMode::QOpenWFDPortMode(QOpenWFDPort *port, WFDPortMode portMode)
    : mPortMode(portMode)
{
    int width = wfdGetPortModeAttribi(port->device()->handle(),port->handle(),portMode,WFD_PORT_MODE_WIDTH);
    int height = wfdGetPortModeAttribi(port->device()->handle(),port->handle(),portMode,WFD_PORT_MODE_HEIGHT);
    mSize = QSize(width,height);

    mRefresh = wfdGetPortModeAttribf(port->device()->handle(),port->handle(),portMode,WFD_PORT_MODE_REFRESH_RATE);
    mFlipMirror = wfdGetPortModeAttribi(port->device()->handle(),port->handle(),portMode,WFD_PORT_MODE_FLIP_MIRROR_SUPPORT);
    mInterlaced = wfdGetPortModeAttribi(port->device()->handle(), port->handle(),portMode,WFD_PORT_MODE_INTERLACED);
}

QDebug operator<<(QDebug s, const QOpenWFDPortMode &portMode)
{
    s.nospace() << "QOpenWFPortMode( " << portMode.size() << " Refreash: " << portMode.refreshRate()
                << " FlipMirror: " << portMode.flipMirror() << " Interlaced: " << portMode.interlaced();
    return s;
}
