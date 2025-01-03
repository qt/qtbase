// Copyright (C) 2016 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QIOSSCREEN_H
#define QIOSSCREEN_H

#include <UIKit/UIKit.h>

#include <qpa/qplatformscreen.h>

@class QIOSOrientationListener;

@interface QUIWindow : UIWindow
@property (nonatomic, readonly) BOOL sendingEvent;
@end

QT_BEGIN_NAMESPACE

class QIOSScreen : public QObject, public QPlatformScreen
{
    Q_OBJECT

public:
    QIOSScreen(UIScreen *screen);
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

    UIScreen *uiScreen() const;
    UIWindow *uiWindow() const;

    void setUpdatesPaused(bool);

    void updateProperties();

private:
    void deliverUpdateRequests() const;

    UIScreen *m_uiScreen;
    UIWindow *m_uiWindow;
    QRect m_geometry;
    QRect m_availableGeometry;
    int m_depth;
    uint m_physicalDpi;
    QSizeF m_physicalSize;
    QIOSOrientationListener *m_orientationListener;
    CADisplayLink *m_displayLink;
};

QT_END_NAMESPACE

#endif
