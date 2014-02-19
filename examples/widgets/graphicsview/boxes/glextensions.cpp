/****************************************************************************
**
** Copyright (C) 2013 Digia Plc and/or its subsidiary(-ies).
** Contact: http://www.qt-project.org/legal
**
** This file is part of the demonstration applications of the Qt Toolkit.
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

#include "glextensions.h"

#define RESOLVE_GL_FUNC(f) ok &= bool((f = (_gl##f) context->getProcAddress(QLatin1String("gl" #f))));

bool GLExtensionFunctions::resolve(const QGLContext *context)
{
    bool ok = true;

    RESOLVE_GL_FUNC(GenFramebuffersEXT)
    RESOLVE_GL_FUNC(GenRenderbuffersEXT)
    RESOLVE_GL_FUNC(BindRenderbufferEXT)
    RESOLVE_GL_FUNC(RenderbufferStorageEXT)
    RESOLVE_GL_FUNC(DeleteFramebuffersEXT)
    RESOLVE_GL_FUNC(DeleteRenderbuffersEXT)
    RESOLVE_GL_FUNC(BindFramebufferEXT)
    RESOLVE_GL_FUNC(FramebufferTexture2DEXT)
    RESOLVE_GL_FUNC(FramebufferRenderbufferEXT)
    RESOLVE_GL_FUNC(CheckFramebufferStatusEXT)

    RESOLVE_GL_FUNC(ActiveTexture)
    RESOLVE_GL_FUNC(TexImage3D)

    RESOLVE_GL_FUNC(GenBuffers)
    RESOLVE_GL_FUNC(BindBuffer)
    RESOLVE_GL_FUNC(BufferData)
    RESOLVE_GL_FUNC(DeleteBuffers)
    RESOLVE_GL_FUNC(MapBuffer)
    RESOLVE_GL_FUNC(UnmapBuffer)

    return ok;
}

bool GLExtensionFunctions::fboSupported() {
    return GenFramebuffersEXT
            && GenRenderbuffersEXT
            && BindRenderbufferEXT
            && RenderbufferStorageEXT
            && DeleteFramebuffersEXT
            && DeleteRenderbuffersEXT
            && BindFramebufferEXT
            && FramebufferTexture2DEXT
            && FramebufferRenderbufferEXT
            && CheckFramebufferStatusEXT;
}

bool GLExtensionFunctions::openGL15Supported() {
    return ActiveTexture
            && TexImage3D
            && GenBuffers
            && BindBuffer
            && BufferData
            && DeleteBuffers
            && MapBuffer
            && UnmapBuffer;
}

#undef RESOLVE_GL_FUNC
