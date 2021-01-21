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

#ifndef QWINRTBACKINGSTORE_H
#define QWINRTBACKINGSTORE_H

#define GL_GLEXT_PROTOTYPES
#include <qpa/qplatformbackingstore.h>
#include <QtCore/QScopedPointer>
#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

class QWinRTScreen;

class QWinRTBackingStorePrivate;
class QWinRTBackingStore : public QPlatformBackingStore
{
public:
    explicit QWinRTBackingStore(QWindow *window);
    ~QWinRTBackingStore() override;
    QPaintDevice *paintDevice() override;
    void beginPaint(const QRegion &) override;
    void endPaint() override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
    void resize(const QSize &size, const QRegion &staticContents) override;
    QImage toImage() const override;

private:
    bool initialize();

    QScopedPointer<QWinRTBackingStorePrivate> d_ptr;
    Q_DECLARE_PRIVATE(QWinRTBackingStore)
};

QT_END_NAMESPACE

#endif // QWINRTBACKINGSTORE_H
