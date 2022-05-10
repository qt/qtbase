// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QOPENGLCONTEXTWINDOW_H
#define QOPENGLCONTEXTWINDOW_H

#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>
#include <QtOpenGL/QOpenGLTextureBlitter>
#include <QtGui/QImage>
#include <QtCore/QVariant>

class QOpenGLContextWindow : public QWindow
{
    Q_OBJECT

public:
    QOpenGLContextWindow();
    ~QOpenGLContextWindow();

    void render();

protected:
    void exposeEvent(QExposeEvent *event);

private:
    qreal dWidth() const { return width() * devicePixelRatio(); }
    qreal dHeight() const { return height() * devicePixelRatio(); }
    void createForeignContext();

    QOpenGLContext *m_context;
    QImage m_image;
    QVariant m_nativeHandle;
    uint m_textureId;
    QOpenGLTextureBlitter *m_blitter;
};

#endif
