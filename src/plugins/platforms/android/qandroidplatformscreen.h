// Copyright (C) 2014 BogDan Vatra <bogdan@kde.org>
// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QANDROIDPLATFORMSCREEN_H
#define QANDROIDPLATFORMSCREEN_H

#include "androidsurfaceclient.h"

#include <QList>
#include <QPainter>
#include <QTimer>
#include <QWaitCondition>
#include <QtCore/QJniObject>
#include <qpa/qplatformscreen.h>
#include <qpa/qplatformscreen_p.h>

#include <android/native_window.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformWindow;

class QAndroidPlatformScreen: public QObject,
                              public QPlatformScreen, public AndroidSurfaceClient,
                              public QNativeInterface::Private::QAndroidScreen
{
    Q_OBJECT
public:
    QAndroidPlatformScreen(const QJniObject &displayObject);
    ~QAndroidPlatformScreen();

    QRect geometry() const override { return QRect(QPoint(), m_size); }
    QRect availableGeometry() const override { return m_availableGeometry; }
    int depth() const override { return m_depth; }
    QImage::Format format() const override { return m_format; }
    QSizeF physicalSize() const override { return m_physicalSize; }

    QString name() const override { return m_name; }
    QList<Mode> modes() const override { return m_modes; }
    int currentMode() const override { return m_currentMode; }
    int preferredMode() const override { return m_currentMode; }
    qreal refreshRate() const override { return m_refreshRate; }
    inline QWindow *topWindow() const;
    QWindow *topLevelAt(const QPoint & p) const override;

    // compositor api
    void addWindow(QAndroidPlatformWindow *window);
    void removeWindow(QAndroidPlatformWindow *window);
    void raise(QAndroidPlatformWindow *window);
    void lower(QAndroidPlatformWindow *window);

    void scheduleUpdate();
    void topWindowChanged(QWindow *w);
    int rasterSurfaces();
    int displayId() const override;

public slots:
    void setDirty(const QRect &rect);
    void setPhysicalSize(const QSize &size);
    void setAvailableGeometry(const QRect &rect);
    void setSize(const QSize &size);
    void setSizeParameters(const QSize &physicalSize, const QSize &size,
                           const QRect &availableGeometry);
    void setRefreshRate(qreal refreshRate);
    void setOrientation(Qt::ScreenOrientation orientation);

protected:
    bool event(QEvent *event) override;

    typedef QList<QAndroidPlatformWindow *> WindowStackType;
    WindowStackType m_windowStack;
    QRect m_dirtyRect;
    bool m_updatePending = false;

    QRect m_availableGeometry;
    int m_depth;
    QImage::Format m_format;
    QSizeF m_physicalSize;
    qreal m_refreshRate;
    QString m_name;
    QList<Mode> m_modes;
    int m_currentMode = 0;
    int m_displayId = -1;

private:
    QDpi logicalDpi() const override;
    QDpi logicalBaseDpi() const override;
    Qt::ScreenOrientation orientation() const override;
    Qt::ScreenOrientation nativeOrientation() const override;
    QPixmap grabWindow(WId window, int x, int y, int width, int height) const override;
    void surfaceChanged(JNIEnv *env, jobject surface, int w, int h) override;
    void releaseSurface();
    void applicationStateChanged(Qt::ApplicationState);
    QPixmap doScreenShot(QRect grabRect = QRect());

private slots:
    void doRedraw(QImage *screenGrabImage = nullptr);

private:
    int m_surfaceId = -1;
    QAtomicInt m_rasterSurfaces = 0;
    ANativeWindow* m_nativeSurface = nullptr;
    QWaitCondition m_surfaceWaitCondition;
    QSize m_size;

    QImage m_lastScreenshot;
    QImage::Format m_pixelFormat = QImage::Format_RGBA8888_Premultiplied;
    bool m_repaintOccurred = false;
};

QT_END_NAMESPACE
#endif
