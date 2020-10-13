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

#ifndef QANDROIDPLATFORMSCREEN_H
#define QANDROIDPLATFORMSCREEN_H

#include "androidsurfaceclient.h"

#include <QList>
#include <QPainter>
#include <QTimer>
#include <QWaitCondition>
#include <QtCore/QJniObject>
#include <qpa/qplatformscreen.h>

#include <android/native_window.h>

QT_BEGIN_NAMESPACE

class QAndroidPlatformWindow;

class QAndroidPlatformScreen: public QObject, public QPlatformScreen, public AndroidSurfaceClient
{
    Q_OBJECT
public:
    QAndroidPlatformScreen();
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

public slots:
    void setDirty(const QRect &rect);
    void setPhysicalSize(const QSize &size);
    void setAvailableGeometry(const QRect &rect);
    void setSize(const QSize &size);
    void setRefreshRate(qreal refreshRate);

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
    int m_id = -1;
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
