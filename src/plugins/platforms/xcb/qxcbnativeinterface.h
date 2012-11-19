/****************************************************************************
**
** Copyright (C) 2012 Digia Plc and/or its subsidiary(-ies).
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

#ifndef QXCBNATIVEINTERFACE_H
#define QXCBNATIVEINTERFACE_H

#include <qpa/qplatformnativeinterface.h>

QT_BEGIN_NAMESPACE

class QWidget;
class QXcbScreen;

class QXcbNativeInterface : public QPlatformNativeInterface
{
public:
    enum ResourceType {
        Display,
        EglDisplay,
        Connection,
        Screen,
        GraphicsDevice,
        EglContext,
        GLXContext
    };

    QXcbNativeInterface();

    void *nativeResourceForContext(const QByteArray &resourceString, QOpenGLContext *context);
    void *nativeResourceForScreen(const QByteArray &resource, QScreen *screen);
    void *nativeResourceForWindow(const QByteArray &resourceString, QWindow *window);

    NativeResourceForContextFunction nativeResourceFunctionForContext(const QByteArray &resource);

    inline const QByteArray &genericEventFilterType() const { return m_genericEventFilterType; }

    void *displayForWindow(QWindow *window);
    void *eglDisplayForWindow(QWindow *window);
    void *connectionForWindow(QWindow *window);
    void *screenForWindow(QWindow *window);
    void *graphicsDeviceForWindow(QWindow *window);
    static void *eglContextForContext(QOpenGLContext *context);
    static void *glxContextForContext(QOpenGLContext *context);

private:
    const QByteArray m_genericEventFilterType;

    static QXcbScreen *qPlatformScreenForWindow(QWindow *window);
};

QT_END_NAMESPACE

#endif // QXCBNATIVEINTERFACE_H
