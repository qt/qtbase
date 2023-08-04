// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

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

    QMargins safeAreaMargins() const override;

    void propagateSizeHints() override;
    void requestActivateWindow() override;
    void updateSystemUiVisibility();
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
