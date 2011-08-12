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

#include "qdirectfbglcontext.h"

#include <directfbgl.h>

#include <QDebug>

QDirectFbGLContext::QDirectFbGLContext(IDirectFBGL *glContext)
    : m_dfbGlContext(glContext)
{
    DFBResult result;
    DFBGLAttributes glAttribs;
    result = m_dfbGlContext->GetAttributes(glContext, &glAttribs);
    if (result == DFB_OK) {
        m_windowFormat.setDepthBufferSize(glAttribs.depth_size);
        m_windowFormat.setStencilBufferSize(glAttribs.stencil_size);

        m_windowFormat.setRedBufferSize(glAttribs.red_size);
        m_windowFormat.setGreenBufferSize(glAttribs.green_size);
        m_windowFormat.setBlueBufferSize(glAttribs.blue_size);
        m_windowFormat.setAlphaBufferSize(glAttribs.alpha_size);

        m_windowFormat.setAccumBufferSize(glAttribs.accum_red_size);
        m_windowFormat.setAlpha(glAttribs.accum_alpha_size);

        m_windowFormat.setDoubleBuffer(glAttribs.double_buffer);
        m_windowFormat.setStereo(glAttribs.stereo);
    }
}

void QDirectFbGLContext::makeCurrent()
{
    QPlatformGLContext::makeCurrent();
    m_dfbGlContext->Lock(m_dfbGlContext);
}

void QDirectFbGLContext::doneCurrent()
{
    QPlatformGLContext::doneCurrent();
    m_dfbGlContext->Unlock(m_dfbGlContext);
}

void *QDirectFbGLContext::getProcAddress(const QString &procName)
{
    void *proc;
    DFBResult result = m_dfbGlContext->GetProcAddress(m_dfbGlContext,qPrintable(procName),&proc);
    if (result == DFB_OK)
        return proc;
    return 0;
}

void QDirectFbGLContext::swapBuffers()
{
//    m_dfbGlContext->Unlock(m_dfbGlContext); //maybe not in doneCurrent()
    qDebug() << "Swap buffers";
}

QPlatformWindowFormat QDirectFbGLContext::platformWindowFormat() const
{
    return m_windowFormat;
}
