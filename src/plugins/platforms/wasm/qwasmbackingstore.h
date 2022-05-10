// Copyright (C) 2018 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only

#ifndef QWASMBACKINGSTORE_H
#define QWASMBACKINGSTORE_H

#include <qpa/qplatformbackingstore.h>
#include <QtGui/qimage.h>

QT_BEGIN_NAMESPACE

class QOpenGLTexture;
class QRegion;
class QWasmCompositor;

class QWasmBackingStore : public QPlatformBackingStore
{
public:
    QWasmBackingStore(QWasmCompositor *compositor, QWindow *window);
    ~QWasmBackingStore();
    void destroy();

    QPaintDevice *paintDevice() override;

    void beginPaint(const QRegion &) override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
    void resize(const QSize &size, const QRegion &staticContents) override;
    QImage toImage() const override;
    const QImage &getImageRef() const;

    const QOpenGLTexture *getUpdatedTexture();

protected:
    void updateTexture();

private:
    QWasmCompositor *m_compositor;
    QImage m_image;
    QScopedPointer<QOpenGLTexture> m_texture;
    QRegion m_dirty;
    bool m_recreateTexture = false;
};

QT_END_NAMESPACE

#endif // QWASMBACKINGSTORE_H
