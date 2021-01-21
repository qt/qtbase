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

#include "qopenwfdwindow.h"

#include "qopenwfdscreen.h"
#include <qpa/qwindowsysteminterface.h>

QOpenWFDWindow::QOpenWFDWindow(QWindow *window)
    : QPlatformWindow(window)
{

    QPlatformScreen *platformScreen = QPlatformScreen::platformScreenForWindow(window);
    mPort = static_cast<QOpenWFDScreen *>(platformScreen)->port();

    QWindowSystemInterface::handleGeometryChange(window,QRect(QPoint(0,0),mPort->screen()->geometry().size()));
}

QOpenWFDPort *QOpenWFDWindow::port() const
{
    return mPort;
}

