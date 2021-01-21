/****************************************************************************
**
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the QtGui module of the Qt Toolkit.
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

#ifndef QOPENGLTEXTUREBLITTER_H
#define QOPENGLTEXTUREBLITTER_H

#include <QtGui/qtguiglobal.h>

#ifndef QT_NO_OPENGL

#include <QtGui/qopengl.h>
#include <QtGui/QMatrix3x3>
#include <QtGui/QMatrix4x4>

QT_BEGIN_NAMESPACE

class QOpenGLTextureBlitterPrivate;

class Q_GUI_EXPORT QOpenGLTextureBlitter
{
public:
    QOpenGLTextureBlitter();
    ~QOpenGLTextureBlitter();

    enum Origin {
        OriginBottomLeft,
        OriginTopLeft
    };

    bool create();
    bool isCreated() const;
    void destroy();

    bool supportsExternalOESTarget() const;

    void bind(GLenum target = GL_TEXTURE_2D);
    void release();

    void setRedBlueSwizzle(bool swizzle);
    void setOpacity(float opacity);

    void blit(GLuint texture, const QMatrix4x4 &targetTransform, Origin sourceOrigin);
    void blit(GLuint texture, const QMatrix4x4 &targetTransform, const QMatrix3x3 &sourceTransform);

    static QMatrix4x4 targetTransform(const QRectF &target, const QRect &viewport);
    static QMatrix3x3 sourceTransform(const QRectF &subTexture, const QSize &textureSize, Origin origin);

private:
    Q_DISABLE_COPY(QOpenGLTextureBlitter)
    Q_DECLARE_PRIVATE(QOpenGLTextureBlitter)
    QScopedPointer<QOpenGLTextureBlitterPrivate> d_ptr;
};

QT_END_NAMESPACE

#endif

#endif //QOPENGLTEXTUREBLITTER_H
