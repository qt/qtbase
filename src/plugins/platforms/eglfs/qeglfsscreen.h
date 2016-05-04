/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Contact: http://www.qt.io/licensing/
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QEGLFSSCREEN_H
#define QEGLFSSCREEN_H

#include "qeglfsglobal.h"
#include <QtCore/QPointer>
#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

class QEglFSWindow;
class QOpenGLContext;

class Q_EGLFS_EXPORT QEglFSScreen : public QPlatformScreen
{
public:
    QEglFSScreen(EGLDisplay display);
    ~QEglFSScreen();

    QRect geometry() const Q_DECL_OVERRIDE;
    int depth() const Q_DECL_OVERRIDE;
    QImage::Format format() const Q_DECL_OVERRIDE;

    QSizeF physicalSize() const Q_DECL_OVERRIDE;
    QDpi logicalDpi() const Q_DECL_OVERRIDE;
    qreal pixelDensity() const Q_DECL_OVERRIDE;
    Qt::ScreenOrientation nativeOrientation() const Q_DECL_OVERRIDE;
    Qt::ScreenOrientation orientation() const Q_DECL_OVERRIDE;

    QPlatformCursor *cursor() const Q_DECL_OVERRIDE;

    qreal refreshRate() const Q_DECL_OVERRIDE;

    QPixmap grabWindow(WId wid, int x, int y, int width, int height) const Q_DECL_OVERRIDE;

    EGLSurface primarySurface() const { return m_surface; }

    EGLDisplay display() const { return m_dpy; }

    void handleCursorMove(const QPoint &pos);

private:
    void setPrimarySurface(EGLSurface surface);

    EGLDisplay m_dpy;
    QPointer<QWindow> m_pointerWindow;
    EGLSurface m_surface;
    QPlatformCursor *m_cursor;

    friend class QEglFSWindow;
};

QT_END_NAMESPACE

#endif // QEGLFSSCREEN_H
