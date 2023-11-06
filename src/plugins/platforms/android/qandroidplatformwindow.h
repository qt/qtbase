// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef ANDROIDPLATFORMWINDOW_H
#define ANDROIDPLATFORMWINDOW_H
#include <qobject.h>
#include <qrect.h>
#include <qpa/qplatformwindow.h>
#include <QtCore/qjnienvironment.h>
#include <QtCore/qjniobject.h>
#include <QtCore/qjnitypes.h>
#include <QtCore/qloggingcategory.h>
#include <QtCore/qmutex.h>
#include <QtCore/qwaitcondition.h>
#include <jni.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_JNI_CLASS(Surface, "android/view/Surface")

class QAndroidPlatformScreen;

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
    inline bool isRaster() const { return m_isRaster; }
    bool isExposed() const override;

    virtual void applicationStateChanged(Qt::ApplicationState);

    void onSurfaceChanged(QtJniTypes::Surface surface);

protected:
    void setGeometry(const QRect &rect) override;
    void lockSurface() { m_surfaceMutex.lock(); }
    void unlockSurface() { m_surfaceMutex.unlock(); }
    void createSurface();
    void destroySurface();
    void setSurfaceGeometry(const QRect &rect);
    void sendExpose();

protected:
    Qt::WindowFlags m_windowFlags;
    Qt::WindowStates m_windowState;
    bool m_isRaster;

    WId m_windowId;
    // The Android Surface, accessed from multiple threads, guarded by m_surfaceMutex
    QtJniTypes::Surface m_androidSurfaceObject;
    QWaitCondition m_surfaceWaitCondition;
    int m_nativeSurfaceId = -1;
    QMutex m_surfaceMutex;
};

QT_END_NAMESPACE
#endif // ANDROIDPLATFORMWINDOW_H
