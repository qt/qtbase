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

#ifndef QXCBGLINTEGRATION_H

#include "qxcbexport.h"
#include "qxcbwindow.h"

#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

class QPlatformOffscreenSurface;
class QOffscreenSurface;
class QXcbNativeInterfaceHandler;

Q_XCB_EXPORT Q_DECLARE_LOGGING_CATEGORY(QT_XCB_GLINTEGRATION)

class Q_XCB_EXPORT QXcbGlIntegration
{
public:
    QXcbGlIntegration();
    virtual ~QXcbGlIntegration();
    virtual bool initialize(QXcbConnection *connection) = 0;

    virtual bool supportsThreadedOpenGL() const { return false; }
    virtual bool handleXcbEvent(xcb_generic_event_t *event, uint responseType);

    virtual QXcbWindow *createWindow(QWindow *window) const = 0;
    virtual QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const = 0;
    virtual QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const = 0;

    virtual QXcbNativeInterfaceHandler *nativeInterfaceHandler() const  { return Q_NULLPTR; }
};

QT_END_NAMESPACE

#endif //QXCBGLINTEGRATION_H
