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

#ifndef QHTML5WINDOW_H
#define QHTML5WINDOW_H

#include "qhtml5integration.h"
#include <qpa/qplatformwindow.h>
#include <emscripten/html5.h>
#include "qhtml5backingstore.h"
#include "qhtml5screen.h"
#include "qhtml5compositor.h"

QT_BEGIN_NAMESPACE

class QHtml5Compositor;

class QHtml5Window : public QPlatformWindow
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

    QHtml5Window(QWindow *w, QHtml5Compositor* compositor);
    ~QHtml5Window();

    void create();

    void setGeometry(const QRect &) override;
    void setVisible(bool visible) override;
    QMargins frameMargins() const override;

    WId winId() const override;

    void propagateSizeHints() override;
    void raise() override;
    void lower() override;
    QRect normalGeometry() const override;

    QHTML5Screen *platformScreen() const;
    void setBackingStore(QHTML5BackingStore *store) { mBackingStore = store; }
    QHTML5BackingStore *backingStore() const { return mBackingStore; }
    QWindow *window() const { return mWindow; }

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
    QHtml5Compositor::SubControls activeSubControl() const;

    void setWindowState(Qt::WindowStates state) override;
    bool setKeyboardGrabEnabled(bool) override { return false; }
    bool setMouseGrabEnabled(bool) override { return false; }

protected:
    void invalidate();

protected:
    friend class QHtml5Screen;

    QHTML5BackingStore *mBackingStore;
    QWindow* mWindow;
    QRect mOldGeometry;
    Qt::WindowFlags mWindowFlags;
    Qt::WindowState mWindowState;

    WId m_winid;
    bool firstRun;
    QHtml5Compositor *mCompositor;
    bool m_raster;

    QHtml5Compositor::SubControls mActiveControl;
    QRect mNormalGeometry;
};
QT_END_NAMESPACE
#endif // QHTML5WINDOW_H
