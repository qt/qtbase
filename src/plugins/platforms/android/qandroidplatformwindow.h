/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
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

#ifndef ANDROIDPLATFORMWINDOW_H
#define ANDROIDPLATFORMWINDOW_H
#include <qobject.h>
#include <qrect.h>
#include <qpa/qplatformwindow.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformScreen;
class QAndroidPlatformBackingStore;

class QAndroidPlatformWindow: public QPlatformWindow
{
public:
    explicit QAndroidPlatformWindow(QWindow *window);

    void lower() override;
    void raise() override;

    void setVisible(bool visible) override;

    void setWindowState(Qt::WindowState state) override;
    void setWindowFlags(Qt::WindowFlags flags) override;
    Qt::WindowFlags windowFlags() const;
    void setParent(const QPlatformWindow *window) override;
    WId winId() const override { return m_windowId; }

    QAndroidPlatformScreen *platformScreen() const;

    void propagateSizeHints() override;
    void requestActivateWindow() override;
    void updateStatusBarVisibility();
    inline bool isRaster() const {
        if ((window()->flags() & Qt::ForeignWindow) == Qt::ForeignWindow)
            return false;

        return window()->surfaceType() == QSurface::RasterSurface
            || window()->surfaceType() == QSurface::RasterGLSurface;
    }
    bool isExposed() const override;

    virtual void applicationStateChanged(Qt::ApplicationState);

    void setBackingStore(QAndroidPlatformBackingStore *store) { m_backingStore = store; }
    QAndroidPlatformBackingStore *backingStore() const { return m_backingStore; }

    virtual void repaint(const QRegion &) { }

protected:
    void setGeometry(const QRect &rect) override;

protected:
    Qt::WindowFlags m_windowFlags;
    Qt::WindowState m_windowState;

    WId m_windowId;

    QAndroidPlatformBackingStore *m_backingStore = nullptr;
};

QT_END_NAMESPACE
#endif // ANDROIDPLATFORMWINDOW_H
