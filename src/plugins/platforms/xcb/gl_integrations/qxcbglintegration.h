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

#ifndef QXCBGLINTEGRATION_H

#include "qxcbexport.h"
#include "qxcbwindow.h"

#include <QtCore/QLoggingCategory>

QT_BEGIN_NAMESPACE

class QPlatformOffscreenSurface;
class QOffscreenSurface;
class QXcbNativeInterfaceHandler;

Q_XCB_EXPORT Q_DECLARE_LOGGING_CATEGORY(lcQpaGl)

class Q_XCB_EXPORT QXcbGlIntegration
{
public:
    QXcbGlIntegration();
    virtual ~QXcbGlIntegration();
    virtual bool initialize(QXcbConnection *connection) = 0;

    virtual bool supportsThreadedOpenGL() const { return false; }
    virtual bool supportsSwitchableWidgetComposition()  const { return true; }
    virtual bool handleXcbEvent(xcb_generic_event_t *event, uint responseType);

    virtual QXcbWindow *createWindow(QWindow *window) const = 0;
#ifndef QT_NO_OPENGL
    virtual QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const = 0;
#endif
    virtual QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const = 0;

    virtual QXcbNativeInterfaceHandler *nativeInterfaceHandler() const  { return nullptr; }
};

QT_END_NAMESPACE

#endif //QXCBGLINTEGRATION_H
