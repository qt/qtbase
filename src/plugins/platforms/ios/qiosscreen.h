// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSSCREEN_H
#define QIOSSCREEN_H

#include <UIKit/UIKit.h>

#include <qpa/qplatformscreen.h>

@class QIOSOrientationListener;

QT_BEGIN_NAMESPACE

class QIOSScreen : public QObject, public QPlatformScreen
{
    Q_OBJECT

public:
#if !defined(Q_OS_VISIONOS)
    QIOSScreen(UIScreen *screen);
#else
    QIOSScreen();
#endif
    ~QIOSScreen();

    QString name() const override;

    QRect geometry() const override;
    QRect availableGeometry() const override;
    int depth() const override;
    QImage::Format format() const override;
    QSizeF physicalSize() const override;
    QDpi logicalBaseDpi() const override;
    qreal devicePixelRatio() const override;
    qreal refreshRate() const override;

    Qt::ScreenOrientation nativeOrientation() const override;
    Qt::ScreenOrientation orientation() const override;

    QPixmap grabWindow(WId window, int x, int y, int width, int height) const override;

#if !defined(Q_OS_VISIONOS)
    UIScreen *uiScreen() const;
#endif

    void setUpdatesPaused(bool);

    void updateProperties();

private:
    void deliverUpdateRequests() const;

#if !defined(Q_OS_VISIONOS)
    UIScreen *m_uiScreen = nullptr;
#endif
    QRect m_geometry;
    QRect m_availableGeometry;
    int m_depth;
#if !defined(Q_OS_VISIONOS)
    uint m_physicalDpi;
#endif
    QSizeF m_physicalSize;
    CADisplayLink *m_displayLink = nullptr;
};

QT_END_NAMESPACE

#endif
