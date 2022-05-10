// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#ifndef QOPENGLTEXTUREBLITWINDOW_H
#define QOPENGLTEXTUREBLITWINDOW_H

#include <QtGui/QWindow>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOpenGLTextureBlitter>

class QOpenGLTextureBlitWindow : public QWindow
{
    Q_OBJECT
public:
    QOpenGLTextureBlitWindow();

    void render();
protected:
    void exposeEvent(QExposeEvent *event);
    void resizeEvent(QResizeEvent *event);

private:
    qreal dWidth() const { return width() * devicePixelRatio(); }
    qreal dHeight() const { return height() * devicePixelRatio(); }

    QScopedPointer<QOpenGLContext> m_context;
    QOpenGLTextureBlitter m_blitter;
    QImage m_image;
    QImage m_image_mirrord;
};

#endif
