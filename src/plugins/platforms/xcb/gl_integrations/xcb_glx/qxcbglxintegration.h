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

#ifndef QXCBGLXINTEGRATION_H
#define QXCBGLXINTEGRATION_H

#include "qxcbglintegration.h"

QT_BEGIN_NAMESPACE

class QXcbNativeInterfaceHandler;

class QXcbGlxIntegration : public QXcbGlIntegration
{
public:
    QXcbGlxIntegration();
    ~QXcbGlxIntegration();

    bool initialize(QXcbConnection *connection) Q_DECL_OVERRIDE;
    bool handleXcbEvent(xcb_generic_event_t *event, uint responseType) Q_DECL_OVERRIDE;

    QXcbWindow *createWindow(QWindow *window) const Q_DECL_OVERRIDE;
    QPlatformOpenGLContext *createPlatformOpenGLContext(QOpenGLContext *context) const Q_DECL_OVERRIDE;
    QPlatformOffscreenSurface *createPlatformOffscreenSurface(QOffscreenSurface *surface) const Q_DECL_OVERRIDE;

    virtual bool supportsThreadedOpenGL() const Q_DECL_OVERRIDE;

private:
    QXcbConnection *m_connection;
    uint32_t m_glx_first_event;

    QScopedPointer<QXcbNativeInterfaceHandler> m_native_interface_handler;
};

QT_END_NAMESPACE

#endif //QXCBGLXINTEGRATION_H
