// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only


#include "qminimalbackingstore.h"
#include "qminimalintegration.h"
#include "qscreen.h"
#include <QtCore/qdebug.h>
#include <qpa/qplatformscreen.h>
#include <private/qguiapplication_p.h>

QT_BEGIN_NAMESPACE

using namespace Qt::StringLiterals;

QMinimalBackingStore::QMinimalBackingStore(QWindow *window)
    : QPlatformBackingStore(window)
    , mDebug(QMinimalIntegration::instance()->options() & QMinimalIntegration::DebugBackingStore)
{
    if (mDebug)
        qDebug() << "QMinimalBackingStore::QMinimalBackingStore:" << (quintptr)this;
}

QMinimalBackingStore::~QMinimalBackingStore()
{
}

QPaintDevice *QMinimalBackingStore::paintDevice()
{
    if (mDebug)
        qDebug("QMinimalBackingStore::paintDevice");

    return &mImage;
}

void QMinimalBackingStore::flush(QWindow *window, const QRegion &region, const QPoint &offset)
{
    Q_UNUSED(window);
    Q_UNUSED(region);
    Q_UNUSED(offset);

    if (mDebug) {
        static int c = 0;
        QString filename = QString("output%1.png").arg(c++, 4, 10, QChar(u'0'));
        qDebug() << "QMinimalBackingStore::flush() saving contents to" << filename.toLocal8Bit().constData();
        mImage.save(filename);
    }
}

void QMinimalBackingStore::resize(const QSize &size, const QRegion &)
{
    QImage::Format format = QGuiApplication::primaryScreen()->handle()->format();
    if (mImage.size() != size)
        mImage = QImage(size, format);
}

QT_END_NAMESPACE
