/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** No Commercial Usage
** This file contains pre-release code and may not be distributed.
** You may use this file in accordance with the terms and conditions
** contained in the Technology Preview License Agreement accompanying
** this package.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPL included in the
** packaging of this file.  Please review the following information to
** ensure the GNU Lesser General Public License version 2.1 requirements
** will be met: http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights.  These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** If you have questions regarding the use of this file, please contact
** Nokia at qt-info@nokia.com.
**
**
**
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QEGLSCREEN_H
#define QEGLSCREEN_H

#include <QPlatformScreen>

#include <QtCore/QTextStream>

#include <EGL/egl.h>

QT_BEGIN_NAMESPACE

class QPlatformGLContext;

class QEglFSScreen : public QPlatformScreen //huh: FullScreenScreen ;) just to follow namespace
{
public:
    QEglFSScreen(EGLNativeDisplayType display);
    ~QEglFSScreen() {}

    QRect geometry() const;
    int depth() const;
    QImage::Format format() const;

    QPlatformGLContext *platformContext() const;

private:
    void createAndSetPlatformContext() const;
    void createAndSetPlatformContext();

    QRect m_geometry;
    int m_depth;
    QImage::Format m_format;
    QPlatformGLContext *m_platformContext;
    EGLDisplay m_dpy;
    EGLSurface m_surface;
};

QT_END_NAMESPACE
#endif // QEGLSCREEN_H
