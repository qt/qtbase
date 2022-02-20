/****************************************************************************
**
** Copyright (C) 2018 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:GPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3 or (at your option) any later version
** approved by the KDE Free Qt Foundation. The licenses are as published by
** the Free Software Foundation and appearing in the file LICENSE.GPL3
** included in the packaging of this file. Please review the following
** information to ensure the GNU General Public License requirements will
** be met: https://www.gnu.org/licenses/gpl-3.0.html.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QWASMCOMPOSITOR_H
#define QWASMCOMPOSITOR_H

#include <QtGui/qregion.h>
#include <qpa/qplatformwindow.h>

#include <QtOpenGL/qopengltextureblitter.h>
#include <QtGui/qpalette.h>
#include <QtGui/qpainter.h>
#include <QtGui/qinputdevice.h>

#include <QPointer>
#include <QPointingDevice>

#include <emscripten/html5.h>

QT_BEGIN_NAMESPACE

class QWasmWindow;
class QWasmScreen;
class QOpenGLContext;
class QOpenGLTexture;
class QWasmEventTranslator;

class QWasmCompositedWindow
{
public:
    QWasmCompositedWindow();

    QWasmWindow *window;
    QWasmWindow *parentWindow;
    QRegion damage;
    bool flushPending;
    bool visible;
    QList<QWasmWindow *> childWindows;
};

class QWasmCompositor : public QObject
{
    Q_OBJECT
public:
    QWasmCompositor(QWasmScreen *screen);
    ~QWasmCompositor();
    void deregisterEventHandlers();
    void destroy();

    enum QWasmSubControl {
        SC_None =                  0x00000000,
        SC_TitleBarSysMenu =       0x00000001,
        SC_TitleBarMinButton =     0x00000002,
        SC_TitleBarMaxButton =     0x00000004,
        SC_TitleBarCloseButton =   0x00000008,
        SC_TitleBarNormalButton =  0x00000010,
        SC_TitleBarLabel =         0x00000100
    };
    Q_DECLARE_FLAGS(SubControls, QWasmSubControl)

    enum QWasmStateFlag {
        State_None =               0x00000000,
        State_Enabled =            0x00000001,
        State_Raised =             0x00000002,
        State_Sunken =             0x00000004
    };
    Q_DECLARE_FLAGS(StateFlags, QWasmStateFlag)

    enum ResizeMode {
        ResizeNone,
        ResizeTopLeft,
        ResizeTop,
        ResizeTopRight,
        ResizeRight,
        ResizeBottomRight,
        ResizeBottom,
        ResizeBottomLeft,
        ResizeLeft
    };

    struct QWasmTitleBarOptions {
        QRect rect;
        Qt::WindowFlags flags;
        int state;
        QPalette palette;
        QString titleBarOptionsString;
        QWasmCompositor::SubControls subControls;
        QIcon windowIcon;
    };

    struct QWasmFrameOptions {
        QRect rect;
        int lineWidth;
        QPalette palette;
    };

    void setEnabled(bool enabled);

    void addWindow(QWasmWindow *window, QWasmWindow *parentWindow = nullptr);
    void removeWindow(QWasmWindow *window);

    void setVisible(QWasmWindow *window, bool visible);
    void raise(QWasmWindow *window);
    void lower(QWasmWindow *window);
    void setParent(QWasmWindow *window, QWasmWindow *parent);

    int windowCount() const;

    QWindow *windowAt(QPoint globalPoint, int padding = 0) const;
    QWindow *keyWindow() const;

    static QWasmTitleBarOptions makeTitleBarOptions(const QWasmWindow *window);
    static QRect titlebarRect(QWasmTitleBarOptions tb, QWasmCompositor::SubControls subcontrol);

    QWasmScreen *screen();
    QOpenGLContext *context();

    enum UpdateRequestDeliveryType { ExposeEventDelivery, UpdateRequestDelivery };
    void requestUpdateAllWindows();
    void requestUpdateWindow(QWasmWindow *window, UpdateRequestDeliveryType updateType = ExposeEventDelivery);
    void requestUpdate();
    void deliverUpdateRequests();
    void deliverUpdateRequest(QWasmWindow *window, UpdateRequestDeliveryType updateType);
    void handleBackingStoreFlush();
    bool processMouse(int eventType, const EmscriptenMouseEvent *mouseEvent);
    bool processKeyboard(int eventType, const EmscriptenKeyboardEvent *keyEvent);
    bool processWheel(int eventType, const EmscriptenWheelEvent *wheelEvent);
    int handleTouch(int eventType, const EmscriptenTouchEvent *touchEvent);
    void resizeWindow(QWindow *window, QWasmCompositor::ResizeMode mode, QRect startRect, QPoint amount);

    bool processMouseEnter(const EmscriptenMouseEvent *mouseEvent);
    bool processMouseLeave();
    void enterWindow(QWindow* window, const QPoint &localPoint, const QPoint &globalPoint);
    void leaveWindow(QWindow* window);

private slots:
    void frame();

private:
    void initEventHandlers();
    void notifyTopWindowChanged(QWasmWindow *window);
    void drawWindow(QOpenGLTextureBlitter *blitter, QWasmScreen *screen, QWasmWindow *window);
    void drawWindowContent(QOpenGLTextureBlitter *blitter, QWasmScreen *screen, QWasmWindow *window);
    void blit(QOpenGLTextureBlitter *blitter, QWasmScreen *screen, const QOpenGLTexture *texture, QRect targetGeometry);

    void drawWindowDecorations(QOpenGLTextureBlitter *blitter, QWasmScreen *screen, QWasmWindow *window);
    void drwPanelButton();

    QScopedPointer<QOpenGLContext> m_context;
    QScopedPointer<QOpenGLTextureBlitter> m_blitter;

    QHash<QWasmWindow *, QWasmCompositedWindow> m_compositedWindows;
    QList<QWasmWindow *> m_windowStack;
    QRegion m_globalDamage; // damage caused by expose, window close, etc.
    bool m_needComposit;
    bool m_inFlush;
    bool m_inResize;
    bool m_isEnabled;
    QSize m_targetSize;
    qreal m_targetDevicePixelRatio;
    QMap<QWasmWindow *, UpdateRequestDeliveryType> m_requestUpdateWindows;
    bool m_requestUpdateAllWindows = false;
    int m_requestAnimationFrameId = -1;
    bool m_inDeliverUpdateRequest = false;

    QPointer<QWindow> draggedWindow;
    QPointer<QWindow> pressedWindow;
    QPointer<QWindow> lastWindow;
    Qt::MouseButtons pressedButtons;

    QWasmCompositor::ResizeMode resizeMode;
    QPoint resizePoint;
    QRect resizeStartRect;
    QPointingDevice *touchDevice;

    QMap <int, QPointF> pressedTouchIds;

    QCursor overriddenCursor;
    bool isCursorOverridden = false;

    static QPalette makeWindowPalette();

    void drawFrameWindow(QWasmFrameOptions options, QPainter *painter);
    void drawTitlebarWindow(QWasmTitleBarOptions options, QPainter *painter);
    void drawShadePanel(QWasmTitleBarOptions options, QPainter *painter);
    void drawItemPixmap(QPainter *painter, const QRect &rect,
                                    int alignment, const QPixmap &pixmap) const;

    static int keyboard_cb(int eventType, const EmscriptenKeyboardEvent *keyEvent, void *userData);
    static int mouse_cb(int eventType, const EmscriptenMouseEvent *mouseEvent, void *userData);
    static int focus_cb(int eventType, const EmscriptenFocusEvent *focusEvent, void *userData);
    static int wheel_cb(int eventType, const EmscriptenWheelEvent *wheelEvent, void *userData);

    static int touchCallback(int eventType, const EmscriptenTouchEvent *ev, void *userData);

    QWasmEventTranslator *eventTranslator;

    bool mouseInCanvas;
    QPointer<QWindow> windowUnderMouse;
};
Q_DECLARE_OPERATORS_FOR_FLAGS(QWasmCompositor::SubControls)

QT_END_NAMESPACE

#endif
