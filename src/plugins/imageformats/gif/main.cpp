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

#include <qimageiohandler.h>
#include <qstringlist.h>

#include "main.h"

#ifdef QT_NO_IMAGEFORMAT_GIF
#undef QT_NO_IMAGEFORMAT_GIF
#endif
#include <qgifhandler_p.h>

QT_BEGIN_NAMESPACE


QGifPlugin::QGifPlugin()
{
}

QGifPlugin::~QGifPlugin()
{
}

QImageIOPlugin::Capabilities QGifPlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    if (format == "gif" || (device && device->isReadable() && QGifHandler::canRead(device)))
        return Capabilities(CanRead);
    return { };
}

QImageIOHandler *QGifPlugin::create(QIODevice *device, const QByteArray &format) const
{
    QImageIOHandler *handler = new QGifHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}

QT_END_NAMESPACE
