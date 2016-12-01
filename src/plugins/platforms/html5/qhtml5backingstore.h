/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl-3.0.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or (at your option) the GNU General
** Public license version 3 or any later version approved by the KDE Free
** Qt Foundation. The licenses are as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL2 and LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-2.0.html and
** https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QHTML5BACKINGSTORE_H
#define QHTML5BACKINGSTORE_H

#include <qpa/qplatformbackingstore.h>
#include <QtGui/QImage>

QT_BEGIN_NAMESPACE

class QOpenGLTexture;
class QRegion;
class QHtml5Compositor;

class QHTML5BackingStore : public QPlatformBackingStore
{
public:
    QHTML5BackingStore(QHtml5Compositor *compositor, QWindow *window);
    ~QHTML5BackingStore();

    QPaintDevice *paintDevice() override;

    void beginPaint(const QRegion &) override;
    void flush(QWindow *window, const QRegion &region, const QPoint &offset) override;
    void resize(const QSize &size, const QRegion &staticContents) override;
    QImage toImage() const override;
    const QImage &getImageRef() const;

    const QOpenGLTexture* getUpdatedTexture();

protected:
    void updateTexture();

private:
    QHtml5Compositor *mCompositor;

    QImage mImage;

    QScopedPointer<QOpenGLTexture> mTexture;

    QRegion mDirty;

};

QT_END_NAMESPACE

#endif // QHTML5BACKINGSTORE_H
