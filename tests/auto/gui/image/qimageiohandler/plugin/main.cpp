// Copyright (C) 2024 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#include <qimageiohandler.h>
#include <qcoreapplication.h>
#include "../pluginlog.h"

class TestImagePlugin : public QImageIOPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QImageIOHandlerFactoryInterface" FILE "plugin.json")

public:
    QImageIOPlugin::Capabilities capabilities(QIODevice *device, const QByteArray &format) const override;
    QImageIOHandler *create(QIODevice *device, const QByteArray &format = QByteArray()) const override;
};

class MyPngHandler : public QImageIOHandler
{
public:
    bool canRead() const override
    {
        if (canRead(device())) {
            setFormat(device()->peek(1).isUpper() ? "png" : "foo");
            return true;
        }
        return false;
    }

    bool read(QImage *image) override
    {
        QColor col = QColor::fromString(device()->readAll().simplified());
        if (col.isValid()) {
            QImage img(32, 32, QImage::Format_RGB32);
            img.fill(col);
            *image = img;
            return true;
        } else {
            return false;
        }
    }

    static bool canRead(QIODevice *device)
    {
        return device ? QColor::isValidColorName(device->peek(16).simplified()) : false;
    }
};


QImageIOPlugin::Capabilities TestImagePlugin::capabilities(QIODevice *device, const QByteArray &format) const
{
    if (format == "png" || format == "foo" || format == "gif") {
        PluginLog::append("formatname-matched");
        return (format == "gif") ? CanWrite : CanRead;
    }

    Capabilities cap;
    if (!format.isEmpty()) {
        PluginLog::append("formatname-unmatched");
        return cap;
    }

    if (!device->isOpen())
        return cap;

    if (device->isReadable()) {
        if (MyPngHandler::canRead(device)) {
            PluginLog::append("contents-matched");
            cap |= CanRead;
        } else {
            PluginLog::append("contents-unmatched");
        }
    }

    if (device->isWritable())
        cap |= CanWrite;

    return cap;
}

QImageIOHandler *TestImagePlugin::create(QIODevice *device, const QByteArray &format) const
{
    QImageIOHandler *handler = new MyPngHandler;
    handler->setDevice(device);
    handler->setFormat(format);
    return handler;
}

#include "main.moc"
