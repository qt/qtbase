// Copyright (C) 2025 The Qt Company Ltd.
// SPDX-License-Identifier: LicenseRef-Qt-Commercial OR LGPL-3.0-only OR GPL-2.0-only OR GPL-3.0-only

#ifndef QOHOSPLATFORMSCREEN_H
#define QOHOSPLATFORMSCREEN_H

#include <qpa/qplatformscreen.h>
#include <QList>
#include <QPainter>
#include <QTimer>
#include <QWaitCondition>
#include <QtCore/qatomic.h>

#include <functional>
#include <qohosdisplayinfo.h>
#include <qohosplugincore.h>
#include "EGL/eglplatform.h"
#include "qohosplatformcursor.h"
// #include <ohos/native_window.h>

QT_BEGIN_NAMESPACE

class QOhosPlatformWindow;
class QOhosNativeXComponent;

class QOhosPlatformScreen: public QObject, public QPlatformScreen
{
    Q_OBJECT
public:
    QOhosPlatformScreen(
        const QOhosDisplayInfo &displayInfo,
        QOhosSupplier<std::vector<QOhosPlatformScreen *>> platformScreenListSupplier);
    ~QOhosPlatformScreen();

    static QOhosPlatformScreen *fromQScreen(QScreen *screen);

    QRect geometry() const override;
    QRect availableGeometry() const override;
    int depth() const override;
    QImage::Format format() const override;
    QSizeF physicalSize() const override;
    qreal pixelScalingCoefficient()  const;
    QString name() const override;

    QWindow *topLevelAt(const QPoint & p) const override;

    QPlatformCursor *cursor() const override;

    // compositor api
    void notifyWindowVisibleAndActivated(QOhosPlatformWindow *window);
    void removeWindow(QOhosPlatformWindow *window);

    const QOhosDisplayInfo &displayInfo() const;

    QPixmap grabWindow(WId wId, int x, int y, int width, int height) const override;

    void setDisplayInfo(const QOhosDisplayInfo &displayInfo);
    void setAvailableGeometry(const QRect &rect);
    QList<QPlatformScreen *> virtualSiblings() const override;

protected:
    QDpi logicalDpi() const override;
    QDpi logicalBaseDpi() const override;
    Qt::ScreenOrientation orientation() const override;
    Qt::ScreenOrientation nativeOrientation() const override;

private:
    QRect getAvailableArea() const;

    QImage::Format m_format;
    int m_depth;

    void releaseSurface();
    QScopedPointer<QOhosPlatformCursor> m_platformCursor;
    QOhosDisplayInfo m_displayInfo;
    QRect m_availableGeometry;
    QOhosSupplier<std::vector<QOhosPlatformScreen *>> m_platformScreenListSupplier;
};

QT_END_NAMESPACE
#endif
