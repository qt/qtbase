/****************************************************************************
**
** Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
** Copyright (C) 2021 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
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

    void setWindowState(Qt::WindowStates state) override;
    void setWindowFlags(Qt::WindowFlags flags) override;
    Qt::WindowFlags windowFlags() const;
    void setParent(const QPlatformWindow *window) override;
    WId winId() const override { return m_windowId; }

    bool setMouseGrabEnabled(bool grab) override { Q_UNUSED(grab); return false; }
    bool setKeyboardGrabEnabled(bool grab) override { Q_UNUSED(grab); return false; }

    QAndroidPlatformScreen *platformScreen() const;

    void propagateSizeHints() override;
    void requestActivateWindow() override;
    void updateStatusBarVisibility();
    inline bool isRaster() const {
        if (isForeignWindow())
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
    Qt::WindowStates m_windowState;

    WId m_windowId;

    QAndroidPlatformBackingStore *m_backingStore = nullptr;
};

QT_END_NAMESPACE
#endif // ANDROIDPLATFORMWINDOW_H
