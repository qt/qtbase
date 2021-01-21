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

#ifndef QIOSBACKINGSTORE_H
#define QIOSBACKINGSTORE_H

#include <qpa/qplatformbackingstore.h>

#include <QtGraphicsSupport/private/qrasterbackingstore_p.h>

QT_BEGIN_NAMESPACE

class QOpenGLPaintDevice;

class QIOSBackingStore : public QRasterBackingStore
{
public:
    QIOSBackingStore(QWindow *window);
    ~QIOSBackingStore();

    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
};

QT_END_NAMESPACE

#endif // QIOSBACKINGSTORE_H
