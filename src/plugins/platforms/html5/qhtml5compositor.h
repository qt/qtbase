/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QHTML5COMPOSITOR_H
#define QHTML5COMPOSITOR_H

#include <QtGui/QRegion>
#include <qpa/qplatformwindow.h>
#include <QtWidgets/QStyleOptionTitleBar>

QT_BEGIN_NAMESPACE

class QHtml5Window;
class QHTML5Screen;
class QOpenGLContext;
class QOpenGLTextureBlitter;

QStyleOptionTitleBar makeTitleBarOptions(const QHtml5Window *window);

class QHtml5CompositedWindow
{
public:
    QHtml5CompositedWindow();

    QHtml5Window *window;
    QHtml5Window *parentWindow;
    QRegion damage;
    bool flushPending;
    bool visible;
    QList<QHtml5Window *> childWindows;
};

class QHtml5Compositor : public QObject
{
    Q_OBJECT
public:
    QHtml5Compositor();
    ~QHtml5Compositor();

    void addWindow(QHtml5Window *window, QHtml5Window *parentWindow = 0);
    void removeWindow(QHtml5Window *window);
    void setScreen(QHTML5Screen *screen);

    void setVisible(QHtml5Window *window, bool visible);
    void raise(QHtml5Window *window);
    void lower(QHtml5Window *window);
    void setParent(QHtml5Window *window, QHtml5Window *parent);

    //void setFrameBuffer(QWindow *window, QImage *frameBuffer);
    void flush(QHtml5Window *surface, const QRegion &region);
    //void waitForFlushed(QWindow *surface);

    /*
    void beginResize(QSize newSize,
                     qreal newDevicePixelRatio); // call when the frame buffer geometry changes
    void endResize();
    */

    int windowCount() const;
    void requestRedraw();

    QWindow *windowAt(QPoint p, int padding = 0) const;
    QWindow *keyWindow() const;
    //void maybeComposit();
    //void composit();

    bool event(QEvent *event);

private slots:
    void frame();

private:
    //void createFrameBuffer();
    void flush2(const QRegion &region);
    void flushCompletedCallback(int32_t);
    void notifyTopWindowChanged(QHtml5Window* window);

private:
    QImage *m_frameBuffer;
    QScopedPointer<QOpenGLContext> mContext;
    QScopedPointer<QOpenGLTextureBlitter> mBlitter;
    QHTML5Screen *mScreen;

    QHash<QHtml5Window *, QHtml5CompositedWindow> m_compositedWindows;
    QList<QHtml5Window *> m_windowStack;
//    pp::Graphics2D *m_context2D;
//    pp::ImageData *m_imageData2D;
    QRegion globalDamage; // damage caused by expose, window close, etc.
    bool m_needComposit;
    bool m_inFlush;
    bool m_inResize;
    QSize m_targetSize;
    qreal m_targetDevicePixelRatio;

//    pp::CompletionCallbackFactory<QPepperCompositor> m_callbackFactory;
};

QT_END_NAMESPACE

#endif
