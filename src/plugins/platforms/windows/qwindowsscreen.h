/****************************************************************************
**
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

#ifndef QWINDOWSSCREEN_H
#define QWINDOWSSCREEN_H

#include "qtwindowsglobal.h"
#ifdef Q_OS_WINCE
#  include "qplatformfunctions_wince.h"
#endif

#include <QtCore/QList>
#include <QtCore/QVector>
#include <QtCore/QPair>
#include <QtCore/QScopedPointer>
#include <qpa/qplatformscreen.h>

QT_BEGIN_NAMESPACE

struct QWindowsScreenData
{
    enum Flags
    {
        PrimaryScreen = 0x1,
        VirtualDesktop = 0x2,
        LockScreen = 0x4 // Temporary screen existing during user change, etc.
    };

    QWindowsScreenData();

    QRect geometry;
    QRect availableGeometry;
    QDpi dpi;
    QSizeF physicalSizeMM;
    int depth;
    QImage::Format format;
    unsigned flags;
    QString name;
    Qt::ScreenOrientation orientation;
    qreal refreshRateHz;
};

class QWindowsScreen : public QPlatformScreen
{
public:
#ifndef QT_NO_CURSOR
    typedef QScopedPointer<QPlatformCursor> CursorPtr;
#endif

    explicit QWindowsScreen(const QWindowsScreenData &data);

    QRect geometry() const Q_DECL_OVERRIDE { return m_data.geometry; }
    QRect availableGeometry() const Q_DECL_OVERRIDE { return m_data.availableGeometry; }
    int depth() const Q_DECL_OVERRIDE { return m_data.depth; }
    QImage::Format format() const Q_DECL_OVERRIDE { return m_data.format; }
    QSizeF physicalSize() const Q_DECL_OVERRIDE { return m_data.physicalSizeMM; }
    QDpi logicalDpi() const Q_DECL_OVERRIDE { return m_data.dpi; }
    qreal pixelDensity() const Q_DECL_OVERRIDE;
    qreal devicePixelRatio() const Q_DECL_OVERRIDE { return 1.0; }
    qreal refreshRate() const Q_DECL_OVERRIDE { return m_data.refreshRateHz; }
    QString name() const Q_DECL_OVERRIDE { return m_data.name; }
    Qt::ScreenOrientation orientation() const Q_DECL_OVERRIDE { return m_data.orientation; }
    QList<QPlatformScreen *> virtualSiblings() const Q_DECL_OVERRIDE;
    QWindow *topLevelAt(const QPoint &point) const Q_DECL_OVERRIDE;
    static QWindow *windowAt(const QPoint &point, unsigned flags);

    QPixmap grabWindow(WId window, int qX, int qY, int qWidth, int qHeight) const Q_DECL_OVERRIDE;
    QPlatformScreen::SubpixelAntialiasingType subpixelAntialiasingTypeHint() const Q_DECL_OVERRIDE;

    static Qt::ScreenOrientation orientationPreference();
    static bool setOrientationPreference(Qt::ScreenOrientation o);

    inline void handleChanges(const QWindowsScreenData &newData);

#ifndef QT_NO_CURSOR
    QPlatformCursor *cursor() const Q_DECL_OVERRIDE { return m_cursor.data(); }
    const CursorPtr &cursorPtr() const { return m_cursor; }
#else
    QPlatformCursor *cursor() const               { return 0; }
#endif // !QT_NO_CURSOR

    const QWindowsScreenData &data() const  { return m_data; }

private:
    QWindowsScreenData m_data;
#ifndef QT_NO_CURSOR
    const CursorPtr m_cursor;
#endif
};

class QWindowsScreenManager
{
public:
    typedef QList<QWindowsScreen *> WindowsScreenList;

    QWindowsScreenManager();

    void clearScreens();

    bool handleScreenChanges();
    bool handleDisplayChange(WPARAM wParam, LPARAM lParam);
    const WindowsScreenList &screens() const { return m_screens; }

    const QWindowsScreen *screenAtDp(const QPoint &p) const;

private:
    void removeScreen(int index);

    WindowsScreenList m_screens;
    int m_lastDepth;
    WORD m_lastHorizontalResolution;
    WORD m_lastVerticalResolution;
};

QT_END_NAMESPACE

#endif // QWINDOWSSCREEN_H
