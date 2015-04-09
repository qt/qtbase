/****************************************************************************
**
** Copyright (C) 2015 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the Qt for Native Client platform plugin.
**
** $QT_BEGIN_LICENSE:LGPL3$
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
** General Public License version 3 as published by the Free Software
** Foundation and appearing in the file LICENSE.LGPLv3 included in the
** packaging of this file. Please review the following information to
** ensure the GNU Lesser General Public License version 3 requirements
** will be met: https://www.gnu.org/licenses/lgpl.html.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU
** General Public License version 2.0 or later as published by the Free
** Software Foundation and appearing in the file LICENSE.GPL included in
** the packaging of this file. Please review the following information to
** ensure the GNU General Public License version 2.0 requirements will be
** met: http://www.gnu.org/licenses/gpl-2.0.html.
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef QPEPPERGLCONTEXT_H
#define QPEPPERGLCONTEXT_H

#include <qglobal.h>

#ifndef QT_NO_OPENGL

#include <QtCore/QLoggingCategory>
#include <qpa/qplatformopenglcontext.h>

#include <ppapi/cpp/completion_callback.h>
#include <ppapi/cpp/graphics_3d.h>
#include <ppapi/utility/completion_callback_factory.h>

QT_BEGIN_NAMESPACE

Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_GLCONTEXT)

class QPepperInstance;
class QPepperGLContext : public QPlatformOpenGLContext
{
public:
    explicit QPepperGLContext();
    ~QPepperGLContext();

    QSurfaceFormat format() const Q_DECL_OVERRIDE;
    void swapBuffers(QPlatformSurface *) Q_DECL_OVERRIDE;
    bool makeCurrent(QPlatformSurface *) Q_DECL_OVERRIDE;
    void doneCurrent() Q_DECL_OVERRIDE;
    QFunctionPointer getProcAddress(const QByteArray &) Q_DECL_OVERRIDE;
    void flushCallback(int32_t);

private:
    bool initGl();
    QSize m_currentSize;
    pp::Graphics3D m_context;
    bool m_pendingFlush;
    pp::CompletionCallbackFactory<QPepperGLContext> m_callbackFactory;
};

QT_END_NAMESPACE

#endif

#endif
