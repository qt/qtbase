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

#ifndef QFBBACKINGSTORE_P_H
#define QFBBACKINGSTORE_P_H

//
//  W A R N I N G
//  -------------
//
// This file is not part of the Qt API.  It exists purely as an
// implementation detail.  This header file may change from version to
// version without notice, or even be removed.
//
// We mean it.
//

#include <qpa/qplatformbackingstore.h>
#include <QtCore/QMutex>

QT_BEGIN_NAMESPACE

class QFbScreen;
class QFbWindow;
class QWindow;

class QFbBackingStore : public QPlatformBackingStore
{
public:
    QFbBackingStore(QWindow *window);
    ~QFbBackingStore();

    QPaintDevice *paintDevice() override { return &mImage; }
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;

    void resize(const QSize &size, const QRegion &region) override;

    const QImage image();
    QImage toImage() const override;

    void lock();
    void unlock();

    void beginPaint(const QRegion &) override;
    void endPaint() override;

protected:
    friend class QFbWindow;

    QImage mImage;
    QMutex mImageMutex;
};

QT_END_NAMESPACE

#endif // QFBBACKINGSTORE_P_H

