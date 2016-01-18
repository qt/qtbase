/****************************************************************************
**
** Copyright (C) 2016 The Qt Company Ltd.
** Contact: https://www.qt.io/licensing/
**
** This file is part of the demonstration applications of the Qt Toolkit.
**
** $QT_BEGIN_LICENSE:BSD$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see https://www.qt.io/terms-conditions. For further
** information use the contact form at https://www.qt.io/contact-us.
**
** BSD License Usage
** Alternatively, you may use this file under the terms of the BSD license
** as follows:
**
** "Redistribution and use in source and binary forms, with or without
** modification, are permitted provided that the following conditions are
** met:
**   * Redistributions of source code must retain the above copyright
**     notice, this list of conditions and the following disclaimer.
**   * Redistributions in binary form must reproduce the above copyright
**     notice, this list of conditions and the following disclaimer in
**     the documentation and/or other materials provided with the
**     distribution.
**   * Neither the name of The Qt Company Ltd nor the names of its
**     contributors may be used to endorse or promote products derived
**     from this software without specific prior written permission.
**
**
** THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
** "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
** LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
** A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
** OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
** SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
** LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
** DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
** THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
** (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
** OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE."
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
