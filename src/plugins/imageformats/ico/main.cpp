// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#include <qimageiohandler.h>
#include <qdebug.h>

#ifdef QT_NO_IMAGEFORMAT_ICO
#undef QT_NO_IMAGEFORMAT_ICO
#endif
#include "qicohandler.h"

QT_BEGIN_NAMESPACE

class QICOPlugin : public QImageIOPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "ico.json")
public:
    Capabilities capabilities(QIODevice *device, const QByteArray &format) const override;
    QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const override;
};

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

#include "main.moc"
