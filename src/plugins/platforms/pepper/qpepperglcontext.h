/****************************************************************************
**
** Copyright (C) 2011 Nokia Corporation and/or its subsidiary(-ies).
** All rights reserved.
** Contact: Nokia Corporation (qt-info@nokia.com)
**
** This file is part of the plugins of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:LGPL$
** GNU Lesser General Public License Usage
** This file may be used under the terms of the GNU Lesser General Public
** License version 2.1 as published by the Free Software Foundation and
** appearing in the file LICENSE.LGPL included in the packaging of this
** file. Please review the following information to ensure the GNU Lesser
** General Public License version 2.1 requirements will be met:
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** In addition, as a special exception, Nokia gives you certain additional
** rights. These rights are described in the Nokia Qt LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** GNU General Public License Usage
** Alternatively, this file may be used under the terms of the GNU General
** Public License version 3.0 as published by the Free Software Foundation
** and appearing in the file LICENSE.GPL included in the packaging of this
** file. Please review the following information to ensure the GNU General
** Public License version 3.0 requirements will be met:
** http://www.gnu.org/copyleft/gpl.html.
**
** Other Usage
** Alternatively, this file may be used in accordance with the terms and
** conditions contained in a signed written agreement between you and Nokia.
**
**
**
**
**
** $QT_END_LICENSE$
**
****************************************************************************/


#ifndef QPLATFORMGLCONTEXT_PEPPER_H
#define QPLATFORMGLCONTEXT_PEPPER_H

#include <qglobal.h>

#ifndef QT_NO_OPENGL

#include <QtCore/qloggingcategory.h>
#include <qpa/qplatformopenglcontext.h>

#include <ppapi/cpp/graphics_3d.h>
#include <ppapi/cpp/graphics_3d_client.h>
#include <ppapi/cpp/completion_callback.h>
#include <ppapi/utility/completion_callback_factory.h>

Q_DECLARE_LOGGING_CATEGORY(QT_PLATFORM_PEPPER_GLCONTEXT)

class QPepperInstance;
class QPepperGLContext : public QPlatformOpenGLContext
{
public:
    explicit QPepperGLContext();
    virtual ~QPepperGLContext();

    virtual bool makeCurrent(QPlatformSurface *);
    virtual void doneCurrent();
    virtual void swapBuffers(QPlatformSurface *);
    void flushCallback(int32_t);
    virtual QFunctionPointer getProcAddress(const QByteArray&);

    virtual QSurfaceFormat format() const;
private:
    bool initGl();
    QSize m_currentSize;
    pp::Graphics3D m_context;
    bool m_pendingFlush;
    pp::CompletionCallbackFactory<QPepperGLContext> m_callbackFactory;
};

#endif

#endif
