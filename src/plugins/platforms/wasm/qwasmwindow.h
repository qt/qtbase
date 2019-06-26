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

#ifndef QWASMWINDOW_H
#define QWASMWINDOW_H

#include "qwasmintegration.h"
#include <qpa/qplatformwindow.h>
#include <emscripten/html5.h>
#include "qwasmbackingstore.h"
#include "qwasmscreen.h"
#include "qwasmcompositor.h"

QT_BEGIN_NAMESPACE

class QWasmCompositor;

class QWasmWindow : public QPlatformWindow
{
public:
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

    QWasmWindow(QWindow *w, QWasmCompositor *compositor, QWasmBackingStore *backingStore);
    ~QWasmWindow();
    void destroy();

    void initialize() override;

    void setGeometry(const QRect &) override;
    void setVisible(bool visible) override;
    QMargins frameMargins() const override;

    WId winId() const override;

    void propagateSizeHints() override;
    void raise() override;
    void lower() override;
    QRect normalGeometry() const override;
    qreal devicePixelRatio() const override;
    void requestUpdate() override;

    QWasmScreen *platformScreen() const;
    void setBackingStore(QWasmBackingStore *store) { m_backingStore = store; }
    QWasmBackingStore *backingStore() const { return m_backingStore; }
    QWindow *window() const { return m_window; }

    void injectMousePressed(const QPoint &local, const QPoint &global,
                            Qt::MouseButton button, Qt::KeyboardModifiers mods);
    void injectMouseReleased(const QPoint &local, const QPoint &global,
                            Qt::MouseButton button, Qt::KeyboardModifiers mods);

    int titleHeight() const;
    int borderWidth() const;
    QRegion titleGeometry() const;
    QRegion resizeRegion() const;
    bool isPointOnTitle(QPoint point) const;
    bool isPointOnResizeRegion(QPoint point) const;
    ResizeMode resizeModeAtPoint(QPoint point) const;
    QRect maxButtonRect() const;
    QRect minButtonRect() const;
    QRect closeButtonRect() const;
    QRect sysMenuRect() const;
    QRect normButtonRect() const;
    QRegion titleControlRegion() const;
    QWasmCompositor::SubControls activeSubControl() const;

    void setWindowState(Qt::WindowStates state) override;
    bool setKeyboardGrabEnabled(bool) override { return false; }
    bool setMouseGrabEnabled(bool) override { return false; }

protected:
    void invalidate();
    bool hasTitleBar() const;

protected:
    friend class QWasmScreen;

    QWindow* m_window = nullptr;
    QWasmCompositor *m_compositor = nullptr;
    QWasmBackingStore *m_backingStore = nullptr;
    QRect m_normalGeometry {0, 0, 0 ,0};

    Qt::WindowState m_windowState = Qt::WindowNoState;
    QWasmCompositor::SubControls m_activeControl = QWasmCompositor::SC_None;
    WId m_winid = 0;
    bool m_hasTitle = false;
    bool m_needsCompositor = false;
    friend class QWasmCompositor;
    friend class QWasmEventTranslator;
};
QT_END_NAMESPACE
#endif // QWASMWINDOW_H
