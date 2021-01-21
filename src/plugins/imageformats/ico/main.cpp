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

#include "main.h"

QT_BEGIN_NAMESPACE

QImageIOPlugin::Capabilities QICOPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    if (format == "ico" || format == "cur")
        return Capabilities(CanRead | CanWrite);
    if (!format.isEmpty())
        return { };
    if (!device->isOpen())
        return { };

    Capabilities cap;
    if (device->isReadable() && QtIcoHandler::canRead(device))
        cap |= CanRead;
    if (device->isWritable())
        cap |= CanWrite;
    return cap;
}

QImageIOHandler *QICOPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QImageIOHandler *handler = new QtIcoHandler(device);

    handler->setFormat(format);
    return handler;
}

QT_END_NAMESPACE
