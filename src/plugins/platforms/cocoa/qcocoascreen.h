/****************************************************************************
**
** Copyright (C) 2017 The Qt Company Ltd.
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

#ifndef QCOCOASCREEN_H
#define QCOCOASCREEN_H

#include <AppKit/AppKit.h>

#include "qcocoacursor.h"

#include <qpa/qplatformintegration.h>

QT_BEGIN_NAMESPACE

class QCocoaScreen : public QPlatformScreen
{
public:
    QCocoaScreen(int screenIndex);
    ~QCocoaScreen();

    // ----------------------------------------------------
    // Virtual methods overridden from QPlatformScreen
    QPixmap grabWindow(WId window, int x, int y, int width, int height) const override;
    QRect geometry() const override { return m_geometry; }
    QRect availableGeometry() const override { return m_availableGeometry; }
    int depth() const override { return m_depth; }
    QImage::Format format() const override { return m_format; }
    qreal devicePixelRatio() const override;
    QSizeF physicalSize() const override { return m_physicalSize; }
    QDpi logicalDpi() const override { return m_logicalDpi; }
    qreal refreshRate() const override { return m_refreshRate; }
    QString name() const override { return m_name; }
    QPlatformCursor *cursor() const override { return m_cursor; }
    QWindow *topLevelAt(const QPoint &point) const override;
    QList<QPlatformScreen *> virtualSiblings() const override { return m_siblings; }
    QPlatformScreen::SubpixelAntialiasingType subpixelAntialiasingTypeHint() const override;

    // ----------------------------------------------------
    // Additional methods
    void setVirtualSiblings(const QList<QPlatformScreen *> &siblings) { m_siblings = siblings; }
    NSScreen *nativeScreen() const;
    void updateGeometry();

    static QCocoaScreen *primaryScreen();

    static CGPoint mapToNative(const QPointF &pos, QCocoaScreen *screen = QCocoaScreen::primaryScreen());
    static CGRect mapToNative(const QRectF &rect, QCocoaScreen *screen = QCocoaScreen::primaryScreen());
    static QPointF mapFromNative(CGPoint pos, QCocoaScreen *screen = QCocoaScreen::primaryScreen());
    static QRectF mapFromNative(CGRect rect, QCocoaScreen *screen = QCocoaScreen::primaryScreen());

public:
    int m_screenIndex;
    QRect m_geometry;
    QRect m_availableGeometry;
    QDpi m_logicalDpi;
    qreal m_refreshRate;
    int m_depth;
    QString m_name;
    QImage::Format m_format;
    QSizeF m_physicalSize;
    QCocoaCursor *m_cursor;
    QList<QPlatformScreen *> m_siblings;
};

#ifndef QT_NO_DEBUG_STREAM
QDebug operator<<(QDebug debug, const QCocoaScreen *screen);
#endif

QT_END_NAMESPACE

#endif

