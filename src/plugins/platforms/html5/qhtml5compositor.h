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

#include <QOpenGLTextureBlitter>
#include <QOpenGLTexture>
#include <QPalette>
#include <QRect>
#include <QFontMetrics>

QT_BEGIN_NAMESPACE

class QHtml5Window;
class QHTML5Screen;
class QOpenGLContext;
class QOpenGLTextureBlitter;

//
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

    enum QHtml5SubControl {
        SC_None =                  0x00000000,
        SC_TitleBarSysMenu =       0x00000001,
        SC_TitleBarMinButton =     0x00000002,
        SC_TitleBarMaxButton =     0x00000004,
        SC_TitleBarCloseButton =   0x00000008,
        SC_TitleBarNormalButton =  0x00000010,
        SC_TitleBarLabel =         0x00000100
    };
    Q_DECLARE_FLAGS(SubControls, QHtml5SubControl)

    enum QHtml5StateFlag {
        State_None =                0x00000000,
        State_Enabled =             0x00000001,
        State_Raised =              0x00000002,
        State_Sunken =              0x00000004
    };
    Q_DECLARE_FLAGS(StateFlags, QHtml5StateFlag)

    struct QHtml5TitleBarOptions {
        QRect rect;
        Qt::WindowFlags flags;
        int state;
        QPalette palette;
        QString titleBarOptionsString;
        QHtml5Compositor::SubControls subControls;
    //    QFontMetrics fontMetrics;
        // Qt::LayoutDirection direction; ??
    };

    struct QHtml5FrameOptions {
        QRect rect;
        int lineWidth;
        QPalette palette;
    };

    void setEnabled(bool enabled);

    void addWindow(QHtml5Window *window, QHtml5Window *parentWindow = 0);
    void removeWindow(QHtml5Window *window);
    void setScreen(QHTML5Screen *screen);

    void setVisible(QHtml5Window *window, bool visible);
    void raise(QHtml5Window *window);
    void lower(QHtml5Window *window);
    void setParent(QHtml5Window *window, QHtml5Window *parent);

    void flush(QHtml5Window *surface, const QRegion &region);

    int windowCount() const;
    void requestRedraw();

    QWindow *windowAt(QPoint p, int padding = 0) const;
    QWindow *keyWindow() const;

    bool event(QEvent *event);

    static QHtml5TitleBarOptions makeTitleBarOptions(const QHtml5Window *window);
    static QRect titlebarRect(QHtml5TitleBarOptions tb, QHtml5Compositor::SubControls subcontrol);

private slots:
    void frame();

private:
    void createFrameBuffer();
    void flush2(const QRegion &region);
    void flushCompletedCallback(int32_t);
    void notifyTopWindowChanged(QHtml5Window* window);
    void drawWindow(QOpenGLTextureBlitter *blitter, QHTML5Screen *screen, QHtml5Window *window);
    void drawWindowContent(QOpenGLTextureBlitter *blitter, QHTML5Screen *screen, QHtml5Window *window);
    void blit(QOpenGLTextureBlitter *blitter, QHTML5Screen *screen, const QOpenGLTexture *texture, QRect targetGeometry);

    void drawWindowDecorations(QOpenGLTextureBlitter *blitter, QHTML5Screen *screen, QHtml5Window *window);
    void drwPanelButton();
private:
    QImage *m_frameBuffer;
    QScopedPointer<QOpenGLContext> mContext;
    QScopedPointer<QOpenGLTextureBlitter> mBlitter;
    QHTML5Screen *mScreen;

    QHash<QHtml5Window *, QHtml5CompositedWindow> m_compositedWindows;
    QList<QHtml5Window *> m_windowStack;
    QRegion globalDamage; // damage caused by expose, window close, etc.
    bool m_needComposit;
    bool m_inFlush;
    bool m_inResize;
    bool m_isEnabled;
    QSize m_targetSize;
    qreal m_targetDevicePixelRatio;

////////////////////////

    static QPalette makeWindowPalette();

    void drawFrameWindow(QHtml5FrameOptions options, QPainter *painter);
    void drawTitlebarWindow(QHtml5TitleBarOptions options, QPainter *painter);
    void drawShadePanel(QHtml5TitleBarOptions options, QPainter *painter);
    void drawItemPixmap(QPainter *painter, const QRect &rect,
                                    int alignment, const QPixmap &pixmap) const;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QHtml5Compositor::SubControls)

QT_END_NAMESPACE

#endif
