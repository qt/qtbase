// Copyright (C) 2017 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QCOCOASCREEN_H
#define QCOCOASCREEN_H

#include "qcocoacursor.h"

#include <qpa/qplatformintegration.h>
#include <QtCore/private/qcore_mac_p.h>

#include <CoreGraphics/CoreGraphics.h>
#include <CoreVideo/CoreVideo.h>

Q_FORWARD_DECLARE_OBJC_CLASS(NSScreen);
Q_FORWARD_DECLARE_OBJC_CLASS(NSArray);

QT_BEGIN_NAMESPACE

class QCocoaIntegration;

class QCocoaScreen : public QPlatformScreen
{
public:
    ~QCocoaScreen();

    // ----------------------------------------------------
    // Virtual methods overridden from QPlatformScreen
    QPixmap grabWindow(WId window, int x, int y, int width, int height) const override;
    QRect geometry() const override { return m_geometry; }
    QRect availableGeometry() const override { return m_availableGeometry; }
    int depth() const override { return m_depth; }
    QImage::Format format() const override { return m_format; }
    QColorSpace colorSpace() const override { return m_colorSpace; }
    qreal devicePixelRatio() const override { return m_devicePixelRatio; }
    QSizeF physicalSize() const override { return m_physicalSize; }
    QDpi logicalBaseDpi() const override { return QDpi(72, 72); }
    qreal refreshRate() const override { return m_refreshRate; }
    QString name() const override { return m_name; }
    QPlatformCursor *cursor() const override { return m_cursor; }
    QWindow *topLevelAt(const QPoint &point) const override;
    QList<QPlatformScreen *> virtualSiblings() const override;
    QPlatformScreen::SubpixelAntialiasingType subpixelAntialiasingTypeHint() const override;

    // ----------------------------------------------------

    static NSScreen *nativeScreenForDisplayId(CGDirectDisplayID displayId);
    NSScreen *nativeScreen() const;

    bool requestUpdate();
    void deliverUpdateRequests();
    bool isRunningDisplayLink() const;

    static QCocoaScreen *primaryScreen();
    static QCocoaScreen *get(NSScreen *nsScreen);
    static QCocoaScreen *get(CGDirectDisplayID displayId);
    static QCocoaScreen *get(CFUUIDRef uuid);

    static CGPoint mapToNative(const QPointF &pos, QCocoaScreen *screen = QCocoaScreen::primaryScreen());
    static CGRect mapToNative(const QRectF &rect, QCocoaScreen *screen = QCocoaScreen::primaryScreen());
    static QPointF mapFromNative(CGPoint pos, QCocoaScreen *screen = QCocoaScreen::primaryScreen());
    static QRectF mapFromNative(CGRect rect, QCocoaScreen *screen = QCocoaScreen::primaryScreen());

private:
    static void initializeScreens();
    static void updateScreens();
    static void cleanupScreens();

    static QMacNotificationObserver s_screenParameterObserver;
    static CGDisplayReconfigurationCallBack s_displayReconfigurationCallBack;

    static void add(CGDirectDisplayID displayId);
    QCocoaScreen(CGDirectDisplayID displayId);
    void update(CGDirectDisplayID displayId);
    void remove();

    bool isOnline() const;
    bool isMirroring() const;

    CGDirectDisplayID m_displayId = kCGNullDirectDisplay;
    CGDirectDisplayID displayId() const { return m_displayId; }

    QRect m_geometry;
    QRect m_availableGeometry;
    qreal m_refreshRate = 0;
    int m_depth = 0;
    QString m_name;
    QImage::Format m_format;
    QColorSpace m_colorSpace;
    QSizeF m_physicalSize;
    QCocoaCursor *m_cursor;
    qreal m_devicePixelRatio = 0;

    CVDisplayLinkRef m_displayLink = nullptr;
    dispatch_source_t m_displayLinkSource = nullptr;
    QAtomicInt m_pendingUpdates;

    friend class QCocoaIntegration;
    friend class QCocoaWindow;
    friend QDebug operator<<(QDebug debug, const QCocoaScreen *screen);
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QCocoaScreen *screen);
#endif

QT_END_NAMESPACE

#if defined(__OBJC__)

// @compatibility_alias doesn't work with categories or their methods
#define qt_displayId QT_MANGLE_NAMESPACE(qt_displayId)

@interface NSScreen (QtExtras)
@property(readonly) CGDirectDisplayID qt_displayId;
@end
#endif

#endif // QCOCOASCREEN_H
