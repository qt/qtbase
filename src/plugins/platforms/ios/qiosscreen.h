/****************************************************************************
**
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
    QDpi logicalDpi() const override;
    qreal devicePixelRatio() const override;
    qreal refreshRate() const override;

    Qt::ScreenOrientation nativeOrientation() const override;
    Qt::ScreenOrientation orientation() const override;
    void setOrientationUpdateMask(Qt::ScreenOrientations mask) override;

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
