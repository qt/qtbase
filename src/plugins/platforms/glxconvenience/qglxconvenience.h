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

#ifndef QGLXCONVENIENCE_H
#define QGLXCONVENIENCE_H

#include <QWindowFormat>

#include <X11/Xlib.h>
#include <GL/glx.h>

XVisualInfo *qglx_findVisualInfo(Display *display, int screen, const QWindowFormat &format);
GLXFBConfig qglx_findConfig(Display *display, int screen, const QWindowFormat &format, int drawableBit = GLX_WINDOW_BIT);
QWindowFormat qglx_platformWindowFromGLXFBConfig(Display *display, GLXFBConfig config, GLXContext context);
QVector<int> qglx_buildSpec(const QWindowFormat &format, int drawableBit = GLX_WINDOW_BIT);
QWindowFormat qglx_reduceWindowFormat(const QWindowFormat &format, bool *reduced);

#endif // QGLXCONVENIENCE_H
