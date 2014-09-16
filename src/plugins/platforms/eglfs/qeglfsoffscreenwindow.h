/****************************************************************************
**
** Copyright (C) 2014 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and Digia.  For licensing terms and
** conditions see http://qt.digia.com/licensing.  For further information
** use the contact form at http://qt.digia.com/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Digia gives you certain additional
** rights.  These rights are described in the Digia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 3.0 as published by the Free Software
** Foundation and appearing in the file LICENSE.GPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU General Public License version 3.0 requirements will be
** met: http://www.gnu.org/copyleft/gpl.html.
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QEGLFSOFFSCREENWINDOW_H
#define QEGLFSOFFSCREENWINDOW_H

#include <EGL/egl.h>
#include <qpa/qplatformoffscreensurface.h>

QT_BEGIN_NAMESPACE

class QEglFSOffscreenWindow : public QPlatformOffscreenSurface
{
public:
    QEglFSOffscreenWindow(EGLDisplay display, const QSurfaceFormat &format, QOffscreenSurface *offscreenSurface);
    ~QEglFSOffscreenWindow();

    QSurfaceFormat format() const Q_DECL_OVERRIDE { return m_format; }
    bool isValid() const Q_DECL_OVERRIDE { return m_surface != EGL_NO_SURFACE; }

private:
    QSurfaceFormat m_format;
    EGLDisplay m_display;
    EGLSurface m_surface;
    EGLNativeWindowType m_window;
};

QT_END_NAMESPACE

#endif // QEGLFSOFFSCREENWINDOW_H
